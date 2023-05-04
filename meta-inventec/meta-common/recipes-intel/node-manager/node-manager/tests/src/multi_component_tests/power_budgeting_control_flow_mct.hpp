/*
 * INTEL CONFIDENTIAL
 *
 * Copyright 2021 Intel Corporation.
 *
 * This software and the related documents are Intel copyrighted materials,
 * and your use of them is governed by the express license under which they
 * were provided to you ("License"). Unless the License provides otherwise,
 * you may not use, modify, copy, publish, distribute, disclose or transmit
 * this software or the related documents without Intel's prior written
 * permission.
 *
 * This software and the related documents are provided as is, with
 * no express or implied warranties, other than those that are expressly
 * stated in the License.
 */

#pragma once
#include "budgeting/budgeting.hpp"
#include "control/control.hpp"
#include "mocks/devices_manager_mock.hpp"
#include "node_manager.hpp"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace nodemanager;

static const double dcPowerSim_{300.0};
static const double psuEfficiencySim_ = 0.7;

class PowerBudgetingControlParams
{
  public:
    PowerBudgetingControlParams& addLimit(DomainId domainId,
                                          uint8_t componentId, double value,
                                          BudgetingStrategy strategy)
    {
        limits.emplace_back(domainId, componentId, value, strategy);
        return *this;
    }

    PowerBudgetingControlParams& addExpectedKnobValue(KnobType knobType,
                                                      uint8_t componentId,
                                                      double value)
    {
        expectedknobValues.emplace_back(knobType, componentId, value);
        return *this;
    }

    PowerBudgetingControlParams& setCpuPresence(const std::string& value)
    {
        cpuPresence = value;
        return *this;
    }

    PowerBudgetingControlParams& setPciePresence(const std::string& value)
    {
        pciePresence = value;
        return *this;
    }

    PowerBudgetingControlParams&
        setDramCapabilitiesMax(std::vector<double> value)
    {
        dramCapMax = value;
        return *this;
    }

    PowerBudgetingControlParams& setDescription(const std::string& value)
    {
        description = value;
        return *this;
    }

    std::vector<std::tuple<DomainId, uint8_t, double, BudgetingStrategy>>
        getLimits() const
    {
        return limits;
    }

    std::vector<std::tuple<KnobType, uint8_t, double>>
        getExpectedKnobValues() const
    {
        return expectedknobValues;
    }

    std::string getCpuPresence() const
    {
        return cpuPresence;
    }

    std::string getPciePresence() const
    {
        return pciePresence;
    }

    std::vector<double> getDramCapabilitiesMax() const
    {
        return dramCapMax;
    }

    friend std::ostream& operator<<(std::ostream& os,
                                    const PowerBudgetingControlParams& o)
    {
        os << "{ description: " << o.description
           << ", cpuPresence: " << o.cpuPresence
           << ", pciePresence: " << o.pciePresence << ", ... }";
        return os;
    }

  private:
    std::string description;
    std::string cpuPresence = "00000000";
    std::string pciePresence = "00000000";
    std::vector<double> dramCapMax{0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0};
    std::vector<std::tuple<DomainId, uint8_t, double, BudgetingStrategy>>
        limits;
    std::vector<std::tuple<KnobType, uint8_t, double>> expectedknobValues;
};

class PowerBudgetingControlFlowMultiComponentTestBase : public testing::Test
{
  protected:
    PowerBudgetingControlFlowMultiComponentTestBase()
    {
        ON_CALL(
            *devManMock_,
            registerReadingConsumerHelper(
                testing::_, ReadingType::platformPowerEfficiency, testing::_))
            .WillByDefault(testing::SaveArg<0>(&pwrEffReadingConsumer_));
        ON_CALL(*devManMock_,
                registerReadingConsumerHelper(
                    testing::_, ReadingType::cpuPresence, testing::_))
            .WillByDefault(testing::SaveArg<0>(&cpuPresenceReadingConsumer_));
        ON_CALL(*devManMock_,
                registerReadingConsumerHelper(
                    testing::_, ReadingType::pciePresence, testing::_))
            .WillByDefault(testing::SaveArg<0>(&pciPresenceReadingConsumer_));

        ON_CALL(*devManMock_, setKnobValue(testing::_, testing::_, testing::_))
            .WillByDefault(testing::Invoke([this](auto a0, auto a1, auto a2) {
                setKnobs.emplace_back(a0, a1, a2);
            }));

        static std::shared_ptr<ReadingConsumer> tempConsumer;
        ON_CALL(*devManMock_,
                registerReadingConsumerHelper(
                    testing::_, ReadingType::dcPlatformPower, testing::_))
            .WillByDefault(testing::DoAll(
                testing::SaveArg<0>(&tempConsumer),
                testing::InvokeWithoutArgs([this]() {
                    dcPlatformReadingConsumers_.push_back(tempConsumer);
                })));

        dramRcs_ = std::vector<std::shared_ptr<ReadingConsumer>>(kMaxCpuNumber);
        for (DeviceIndex i = 0; i < kMaxCpuNumber; ++i)
        {
            ON_CALL(*devManMock_,
                    registerReadingConsumerHelper(
                        testing::_,
                        ReadingType::dramPackagePowerCapabilitiesMax, i))
                .WillByDefault(
                    testing::SaveArg<0>(&dramRcs_[static_cast<uint8_t>(i)]));
        }
        sut_ = std::make_unique<FakeNodeManager>(devManMock_);
    }
    virtual ~PowerBudgetingControlFlowMultiComponentTestBase() = default;

    class FakeNodeManager : public RunnerIf
    {
      public:
        FakeNodeManager() = delete;
        FakeNodeManager(std::shared_ptr<DevicesManagerIf> dm)
        {
            SimpleDomainDistributors simpleDomainDistributors_;

            for (const auto& config : kSimpleDomainDistributorsConfig)
            {
                simpleDomainDistributors_.emplace_back(
                    config.raplDomainId,
                    std::make_unique<SimpleDomainBudgeting>(
                        std::move(std::make_unique<RegulatorP>(
                            dm, config.regulatorPCoeff,
                            config.regulatorFeedbackReading)),
                        std::move(std::make_unique<EfficiencyHelper>(
                            dm, config.efficiencyReading,
                            config.efficiencyAveragingPeriod)),
                        config.budgetCorrection));
            }

            auto compoundBudgeting_ = std::make_unique<CompoundDomainBudgeting>(
                std::move(simpleDomainDistributors_));

            control_ = std::make_shared<Control>(dm);

            budgeting_ = std::make_unique<Budgeting>(
                dm, std::move(compoundBudgeting_), control_);
        }
        void run() override
        {
            budgeting_->run();
            control_->run();
        }

        std::shared_ptr<Control> control_;
        std::unique_ptr<Budgeting> budgeting_;
    };

    void SetUp() override
    {
    }

    void TearDown() override
    {
        sut_ = nullptr;
    }

    unsigned updateCpuPresenceMap(std::string map)
    {
        const auto tmpMap = std::bitset<kMaxCpuNumber>(map);
        cpuPresenceReadingConsumer_->updateValue(
            static_cast<double>(tmpMap.to_ulong()));
        return tmpMap.count();
    }

    unsigned updatePciePresenceMap(std::string map)
    {
        const auto tmpMap = std::bitset<kMaxCpuNumber>(map);
        pciPresenceReadingConsumer_->updateValue(
            static_cast<double>(tmpMap.to_ulong()));
        return tmpMap.count();
    }

    void updateDramReadings(std::vector<double> values)
    {
        for (size_t i = 0; i < kMaxCpuNumber; ++i)
        {
            dramRcs_[i]->updateValue(values[i]);
        }
    }

    std::shared_ptr<::testing::NiceMock<DevicesManagerMock>> devManMock_ =
        std::make_shared<::testing::NiceMock<DevicesManagerMock>>();
    std::shared_ptr<ReadingConsumer> pwrEffReadingConsumer_;
    std::shared_ptr<ReadingConsumer> cpuPresenceReadingConsumer_;
    std::shared_ptr<ReadingConsumer> pciPresenceReadingConsumer_;
    std::vector<std::shared_ptr<ReadingConsumer>> dcPlatformReadingConsumers_;
    std::vector<std::shared_ptr<ReadingConsumer>> dramRcs_;
    std::vector<std::tuple<KnobType, uint8_t, double>> setKnobs;
    std::unique_ptr<FakeNodeManager> sut_;
};

class PowerBudgetingControlFlowMultiComponentTest
    : public PowerBudgetingControlFlowMultiComponentTestBase,
      public ::testing::WithParamInterface<PowerBudgetingControlParams>
{
  protected:
    virtual void SetUp() override
    {
        PowerBudgetingControlFlowMultiComponentTestBase::SetUp();

        updateCpuPresenceMap(GetParam().getCpuPresence());
        updatePciePresenceMap(GetParam().getPciePresence());
        updateDramReadings(GetParam().getDramCapabilitiesMax());
    }
};

std::vector<PowerBudgetingControlParams> makeTestParamSet(DomainId domainId,
                                                          KnobType knobType)
{
    const auto base =
        PowerBudgetingControlParams()
            .setCpuPresence("00000101")
            .setPciePresence("00000101")
            .setDramCapabilitiesMax({5.0, 0.0, 5.0, 0.0, 0.0, 0.0, 0.0, 0.0});

    return std::vector<PowerBudgetingControlParams>(
        {PowerBudgetingControlParams(base)
             .setDescription("set knobs when limit set to 0")
             .addLimit(domainId, kComponentIdAll, 0.0,
                       BudgetingStrategy::aggressive)
             .addLimit(domainId, 2, 0.0, BudgetingStrategy::aggressive)
             .addLimit(domainId, 0, 20.1, BudgetingStrategy::aggressive)
             .addExpectedKnobValue(knobType, 2, 0.0)
             .addExpectedKnobValue(knobType, 0, 0.0),
         PowerBudgetingControlParams(base)
             .setDescription("set knobs when limit set for two devices")
             .addLimit(domainId, 2, 35.7, BudgetingStrategy::aggressive)
             .addLimit(domainId, 0, 20.1, BudgetingStrategy::aggressive)
             .addExpectedKnobValue(knobType, 2, 35.7)
             .addExpectedKnobValue(knobType, 0, 20.1),
         PowerBudgetingControlParams(base)
             .setDescription("doesn't set knob for non existing device")
             .addLimit(domainId, 1, 35.7, BudgetingStrategy::aggressive)
             .addLimit(domainId, 0, 20.1, BudgetingStrategy::aggressive)
             .addExpectedKnobValue(knobType, 0, 20.1),
         PowerBudgetingControlParams(base)
             .setDescription("set knobs when limit set for domain")
             .addLimit(domainId, kComponentIdAll, 50.0,
                       BudgetingStrategy::aggressive)
             .addExpectedKnobValue(knobType, 2, 25.0)
             .addExpectedKnobValue(knobType, 0, 25.0),
         PowerBudgetingControlParams(base)
             .setDescription("set knobs when limit set for domain and device")
             .addLimit(domainId, kComponentIdAll, 50.0,
                       BudgetingStrategy::aggressive)
             .addLimit(domainId, 2, 35.7, BudgetingStrategy::aggressive)
             .addLimit(domainId, 0, 20.1, BudgetingStrategy::aggressive)
             .addExpectedKnobValue(knobType, 2, 29.9)
             .addExpectedKnobValue(knobType, 0, 20.1),
         PowerBudgetingControlParams(base)
             .setDescription("set knobs when limit set for domain and device")
             .addLimit(domainId, kComponentIdAll, 100.0,
                       BudgetingStrategy::aggressive)
             .addLimit(domainId, 2, 35.7, BudgetingStrategy::aggressive)
             .addLimit(domainId, 0, 20.1, BudgetingStrategy::aggressive)
             .addExpectedKnobValue(knobType, 2, 35.7)
             .addExpectedKnobValue(knobType, 0, 20.1)});
}

INSTANTIATE_TEST_SUITE_P(
    _, PowerBudgetingControlFlowMultiComponentTest,
    testing::Values(PowerBudgetingControlParams()
                        .setDescription("doesn't set knobs when no limitset")
                        .setCpuPresence("00001001")
                        .setPciePresence("00010100")
                        .setDramCapabilitiesMax({5.0, 0.0, 0.0, 5.0, 0.0, 0.0,
                                                 0.0, 0.0})));

INSTANTIATE_TEST_SUITE_P(
    PcieDomain, PowerBudgetingControlFlowMultiComponentTest,
    testing::ValuesIn(makeTestParamSet(DomainId::Pcie, KnobType::PciePower)));

INSTANTIATE_TEST_SUITE_P(MemorySubsystem,
                         PowerBudgetingControlFlowMultiComponentTest,
                         testing::ValuesIn(makeTestParamSet(
                             DomainId::MemorySubsystem, KnobType::DramPower)));

TEST_P(PowerBudgetingControlFlowMultiComponentTest, SetsKnobLimit)
{
    for (size_t iter = 0u; iter < 10; ++iter)
    {
        for (const auto& limit : GetParam().getLimits())
        {
            std::apply(
                &Budgeting::setLimit,
                std::tuple_cat(std::make_tuple(sut_->budgeting_.get()), limit));
        }

        sut_->run();

        ASSERT_THAT(setKnobs, testing::UnorderedElementsAreArray(
                                  GetParam().getExpectedKnobValues()));

        setKnobs.clear();
    }
}

TEST_P(PowerBudgetingControlFlowMultiComponentTest, ResetsKnobLimit)
{
    EXPECT_CALL(*devManMock_, resetKnobValue(testing::_, testing::_))
        .Times(testing::AnyNumber());
    for (const auto& [knobType, componentId, value] :
         GetParam().getExpectedKnobValues())
    {
        EXPECT_CALL(*devManMock_, resetKnobValue(knobType, componentId))
            .Times(1);
    }

    for (const auto& [domainId, componentId, value, strategy] :
         GetParam().getLimits())
    {
        sut_->budgeting_->setLimit(domainId, componentId, value, strategy);
    }
    sut_->run();

    ASSERT_THAT(setKnobs, testing::UnorderedElementsAreArray(
                              GetParam().getExpectedKnobValues()));

    for (const auto& [domainId, componentId, value, strategy] :
         GetParam().getLimits())
    {
        sut_->budgeting_->resetLimit(domainId, componentId, strategy);
    }
    sut_->run();
}

TEST_P(PowerBudgetingControlFlowMultiComponentTest,
       DoesntSetLimitWhenResetBeforeIteration)
{
    for (const auto& [domainId, componentId, value, strategy] :
         GetParam().getLimits())
    {
        sut_->budgeting_->setLimit(domainId, componentId, value, strategy);
        sut_->budgeting_->resetLimit(domainId, componentId, strategy);
    }

    for (size_t iter = 0u; iter < 10; ++iter)
    {
        sut_->run();
    }

    ASSERT_THAT(setKnobs, testing::ElementsAre());
}

class CompoundBudgetingControlFlowMultiComponentTest
    : public PowerBudgetingControlFlowMultiComponentTestBase,
      public ::testing::WithParamInterface<
          std::vector<PowerBudgetingControlParams>>
{
  protected:
    virtual void SetUp() override
    {
        PowerBudgetingControlFlowMultiComponentTestBase::SetUp();

        pwrEffReadingConsumer_->updateValue(psuEfficiencySim_);

        for (auto dcPlatformReadingConsumer_ : dcPlatformReadingConsumers_)
        {
            dcPlatformReadingConsumer_->updateValue(dcPowerSim_);
        }

        updateCpuPresenceMap(GetParam()[0].getCpuPresence());
        updatePciePresenceMap(GetParam()[0].getPciePresence());
        updateDramReadings(GetParam()[0].getDramCapabilitiesMax());
    }
};

std::vector<std::vector<PowerBudgetingControlParams>> makeCompoundTestParamSet()
{
    const double powerLimit_{100.0};
    const double csDelta_{(powerLimit_ - dcPowerSim_) * 0.4};
    const double csFirstRun_{(32767.0 + csDelta_) / 2};
    const double csSecondRun_{csFirstRun_ + csDelta_ / 2};

    const double powerLimitAc_{powerLimit_ * psuEfficiencySim_};
    const double csDeltaAc_{(powerLimitAc_ - dcPowerSim_) * 0.4};
    const double csFirstRunAc_{(32767.0 + csDeltaAc_) / 2};
    const double csSecondRunAc_{csFirstRunAc_ + csDeltaAc_ / 2};

    const auto base =
        PowerBudgetingControlParams()
            .setCpuPresence("00001001")
            .setPciePresence("00010100")
            .setDramCapabilitiesMax({5.0, 0.0, 0.0, 5.0, 0.0, 0.0, 0.0, 0.0});

    return std::vector<std::vector<PowerBudgetingControlParams>>(
        {{PowerBudgetingControlParams(base)
              .setDescription(
                  "set dc total agressive limit and distribite to memory and "
                  "pcie - first run")
              .addLimit(DomainId::DcTotalPower, kComponentIdAll, powerLimit_,
                        BudgetingStrategy::aggressive)
              .addExpectedKnobValue(KnobType::DcPlatformPower, 0, powerLimit_)
              .addExpectedKnobValue(KnobType::PciePower, 2, csFirstRun_)
              .addExpectedKnobValue(KnobType::PciePower, 4, csFirstRun_)
              .addExpectedKnobValue(KnobType::DramPower, 0, csFirstRun_)
              .addExpectedKnobValue(KnobType::DramPower, 3, csFirstRun_),
          PowerBudgetingControlParams(base)
              .setDescription(
                  "set dc total aggressive limit and distribite to memory and "
                  "pcie - second run")
              .addExpectedKnobValue(KnobType::DcPlatformPower, 0, powerLimit_)
              .addExpectedKnobValue(KnobType::PciePower, 2, csSecondRun_)
              .addExpectedKnobValue(KnobType::PciePower, 4, csSecondRun_)
              .addExpectedKnobValue(KnobType::DramPower, 0, csSecondRun_)
              .addExpectedKnobValue(KnobType::DramPower, 3, csSecondRun_)},
         {PowerBudgetingControlParams(base)
              .setDescription(
                  "set ac total agressive limit and distribite to memory and "
                  "pcie - first run")
              .addLimit(DomainId::AcTotalPower, kComponentIdAll, powerLimit_,
                        BudgetingStrategy::aggressive)
              .addExpectedKnobValue(KnobType::DcPlatformPower, 0, powerLimitAc_)
              .addExpectedKnobValue(KnobType::PciePower, 2, csFirstRunAc_)
              .addExpectedKnobValue(KnobType::PciePower, 4, csFirstRunAc_)
              .addExpectedKnobValue(KnobType::DramPower, 0, csFirstRunAc_)
              .addExpectedKnobValue(KnobType::DramPower, 3, csFirstRunAc_),
          PowerBudgetingControlParams(base)
              .setDescription(
                  "set ac total aggressive limit and distribite to memory and "
                  "pcie - second run")
              .addExpectedKnobValue(KnobType::DcPlatformPower, 0, powerLimitAc_)
              .addExpectedKnobValue(KnobType::PciePower, 2, csSecondRunAc_)
              .addExpectedKnobValue(KnobType::PciePower, 4, csSecondRunAc_)
              .addExpectedKnobValue(KnobType::DramPower, 0, csSecondRunAc_)
              .addExpectedKnobValue(KnobType::DramPower, 3, csSecondRunAc_)},
         {PowerBudgetingControlParams(base)
              .setDescription(
                  "set dc total nonagressive limit and pass to psys")
              .addLimit(DomainId::DcTotalPower, kComponentIdAll, powerLimit_,
                        BudgetingStrategy::nonAggressive)
              .addExpectedKnobValue(KnobType::DcPlatformPower, 0, powerLimit_)},
         {PowerBudgetingControlParams(base)
              .setDescription("set dc total agressive high and non aggressive "
                              "0 limit and distribite to memory and "
                              "pcie")
              .addLimit(DomainId::DcTotalPower, kComponentIdAll, 10000,
                        BudgetingStrategy::aggressive)
              .addLimit(DomainId::DcTotalPower, kComponentIdAll, 0,
                        BudgetingStrategy::nonAggressive)
              .addExpectedKnobValue(KnobType::DcPlatformPower, 0, 0)
              .addExpectedKnobValue(KnobType::PciePower, 2, 32767.0 / 2)
              .addExpectedKnobValue(KnobType::PciePower, 4, 32767.0 / 2)
              .addExpectedKnobValue(KnobType::DramPower, 0, 32767.0 / 2)
              .addExpectedKnobValue(KnobType::DramPower, 3, 32767.0 / 2)}});
}

INSTANTIATE_TEST_SUITE_P(DcTotalPower,
                         CompoundBudgetingControlFlowMultiComponentTest,
                         testing::ValuesIn(makeCompoundTestParamSet()));

TEST_P(CompoundBudgetingControlFlowMultiComponentTest, SetsKnobLimit)
{
    for (auto& param : GetParam())
    {
        for (const auto& limit : param.getLimits())
        {
            std::apply(
                &Budgeting::setLimit,
                std::tuple_cat(std::make_tuple(sut_->budgeting_.get()), limit));
        }

        sut_->run();

        ASSERT_THAT(setKnobs, testing::UnorderedElementsAreArray(
                                  param.getExpectedKnobValues()));

        setKnobs.clear();
    }
}

TEST_P(CompoundBudgetingControlFlowMultiComponentTest, ResetsKnobLimit)
{
    EXPECT_CALL(*devManMock_, resetKnobValue(testing::_, testing::_))
        .Times(testing::AnyNumber());
    for (const auto& [knobType, componentId, value] :
         GetParam()[0].getExpectedKnobValues())
    {
        EXPECT_CALL(*devManMock_, resetKnobValue(knobType, componentId))
            .Times(1);
    }

    for (const auto& [domainId, componentId, value, strategy] :
         GetParam()[0].getLimits())
    {
        sut_->budgeting_->setLimit(domainId, componentId, value, strategy);
    }
    sut_->run();

    ASSERT_THAT(setKnobs, testing::UnorderedElementsAreArray(
                              GetParam()[0].getExpectedKnobValues()));

    for (const auto& [domainId, componentId, value, strategy] :
         GetParam()[0].getLimits())
    {
        sut_->budgeting_->resetLimit(domainId, componentId, strategy);
    }
    sut_->run();
}

TEST_P(CompoundBudgetingControlFlowMultiComponentTest,
       DoesntSetLimitWhenResetBeforeIteration)
{
    for (const auto& [domainId, componentId, value, strategy] :
         GetParam()[0].getLimits())
    {
        sut_->budgeting_->setLimit(domainId, componentId, value, strategy);
        sut_->budgeting_->resetLimit(domainId, componentId, strategy);
    }

    for (size_t iter = 0u; iter < 10; ++iter)
    {
        sut_->run();
    }

    ASSERT_THAT(setKnobs, testing::ElementsAre());
}