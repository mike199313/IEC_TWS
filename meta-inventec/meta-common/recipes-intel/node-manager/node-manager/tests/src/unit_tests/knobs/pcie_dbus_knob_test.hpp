/*
 * INTEL CONFIDENTIAL
 *
 * Copyright 2022 Intel Corporation.
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
#include "common_types.hpp"
#include "knobs/pcie_dbus_knob.hpp"
#include "mocks/pldm_entity_provider_mock.hpp"
#include "mocks/sensor_readings_manager_mock.hpp"
#include "stubs/dbus_pldm_effecter_stub.hpp"
#include "utils/dbus_environment.hpp"

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <gtest/internal/gtest-death-test-internal.h>

namespace nodemanager
{

class PcieDbusKnobTest : public testing::Test
{
  public:
    virtual ~PcieDbusKnobTest() = default;

    virtual void SetUp() override
    {
        ON_CALL(*pciePowerCapabilitiesMaxPldmSensorReading_, getValue())
            .WillByDefault(testing::Return(pciePowerCapabilitiesMaxPldmValue));

        ON_CALL(
            *sensorReadingsManager_,
            getAvailableAndValueValidSensorReading(
                SensorReadingType::pciePowerCapabilitiesMaxPldm, deviceIndex))
            .WillByDefault(
                testing::Return(pciePowerCapabilitiesMaxPldmSensorReading_));

        std::string deviceName{"ATS"};
        std::string tid{"1"};
        dbusPldmEffecters_.emplace(
            PcieKnobType::PL1,
            makePldmEffecter(deviceName, tid, deviceIndex, "power", "PL1"));
        dbusPldmEffecters_.emplace(
            PcieKnobType::PL1Tau,
            makePldmEffecter(deviceName, tid, deviceIndex, "time", "Tau_PL1"));
        dbusPldmEffecters_.emplace(
            PcieKnobType::PL2,
            makePldmEffecter(deviceName, tid, deviceIndex, "power", "PL2"));
        dbusPldmEffecters_.emplace(
            PcieKnobType::PL2Tau,
            makePldmEffecter(deviceName, tid, deviceIndex, "time", "Tau_PL2"));

        std::string pvcDeviceName{"PVC"};
        std::string pvcTid{"2"};
        pvcDbusPldmEffecters_.emplace(
            PcieKnobType::PL1, makePldmEffecter(pvcDeviceName, pvcTid,
                                                deviceIndex, "power", "PL1"));
        pvcDbusPldmEffecters_.emplace(PcieKnobType::PL1Tau,
                                      makePldmEffecter(pvcDeviceName, pvcTid,
                                                       deviceIndex, "time",
                                                       "Tau_PL1"));
        pvcDbusPldmEffecters_.emplace(
            PcieKnobType::PL2, makePldmEffecter(pvcDeviceName, pvcTid,
                                                deviceIndex, "power", "PL2"));
        pvcDbusPldmEffecters_.emplace(PcieKnobType::PL2Tau,
                                      makePldmEffecter(pvcDeviceName, pvcTid,
                                                       deviceIndex, "time",
                                                       "Tau_PL2"));

        setPldmProviderGetDeviceNameReturnValue(deviceName);
        setPldmProviderGetTidReturnValue(tid);

        ON_CALL(*pldmEntityProvider_,
                registerDiscoveryDataChangeCallback(testing::_))
            .WillByDefault(
                testing::SaveArg<0>(&(this->storedDiscoveryDataCallback)));

        sut_ = std::make_shared<PcieDbusKnob>(
            KnobType::PciePower, deviceIndex, pldmEntityProvider_,
            DbusEnvironment::getBus(), sensorReadingsManager_,
            DbusEnvironment::serviceName());

        ASSERT_TRUE(waitForAvailability(true, true));
    }

    virtual void TearDown() override
    {
        ASSERT_TRUE(DbusEnvironment::waitForAllFutures());
        sut_ = nullptr;
        ASSERT_TRUE(DbusEnvironment::waitForAllFutures());
    }

    bool waitForAvailability(bool pl1, bool pl2)
    {
        return waitForTask([this, pl1, pl2]() {
            nlohmann::json out;
            nlohmann::json knobsPcieDbus;
            nlohmann::json pciePower;

            sut_->reportStatus(out);
            knobsPcieDbus = out["Knobs-pci-dbus"].get<nlohmann::json>();
            pciePower = knobsPcieDbus["PciePower"].get<nlohmann::json>()[0];

            return pciePower["StatusPL1"].get<bool>() == pl1 &&
                   pciePower["StatusPL2"].get<bool>() == pl2;
        });
    }

    bool waitForValueSet(double value, std::chrono::milliseconds timeout =
                                           std::chrono::milliseconds(1000))
    {
        return waitForTask([this, value]() {
            nlohmann::json out;
            nlohmann::json knobsPcieDbus;
            nlohmann::json pciePower;

            sut_->reportStatus(out);
            knobsPcieDbus = out["Knobs-pci-dbus"].get<nlohmann::json>();
            pciePower = knobsPcieDbus["PciePower"].get<nlohmann::json>()[0];

            try
            {
                return pciePower["Value"].get<double>() == value;
            }
            catch (const std::exception& e)
            {
                return false;
            }
        });
    }

    bool waitForHealthSet(NmHealth health, std::chrono::milliseconds timeout =
                                               std::chrono::milliseconds(1000))
    {
        return waitForTask(
            [this, health]() { return sut_->getHealth() == health; });
    }

    bool waitForIsKnobSet(bool state, std::chrono::milliseconds timeout =
                                          std::chrono::milliseconds(1000))
    {
        return waitForTask(
            [this, state]() { return sut_->isKnobSet() == state; });
    }

    void setDbusSetEffecterExpectation(
        PcieKnobType pcieKnobType, double limitValue,
        std::map<PcieKnobType, std::unique_ptr<DbusPldmEffecterStub>>&
            effectersGroup,
        testing::Sequence s = testing::Sequence())
    {
        if (pcieKnobType == PcieKnobType::PL1)
        {
            EXPECT_CALL(*effectersGroup[PcieKnobType::PL1],
                        setEffecter(limitValue))
                .InSequence(s);
            EXPECT_CALL(*effectersGroup[PcieKnobType::PL1Tau],
                        setEffecter(testing::_))
                .InSequence(s);
        }
        else if (pcieKnobType == PcieKnobType::PL2)
        {
            EXPECT_CALL(*effectersGroup[PcieKnobType::PL2],
                        setEffecter(limitValue))
                .InSequence(s);
            EXPECT_CALL(*effectersGroup[PcieKnobType::PL2Tau],
                        setEffecter(testing::_))
                .InSequence(s);
        }
    }

    template <class T = void>
    static auto throwSdBusError(const std::errc err, const std::string msg)
    {
        return testing::InvokeWithoutArgs([err, msg]() -> T {
            throw sdbusplus::exception::SdBusError(static_cast<int>(err),
                                                   msg.c_str());
            return T{};
        });
    }

    void setErrorLimitExpectation(
        PcieKnobType pcieKnobType, double limitValue,
        std::map<PcieKnobType, std::unique_ptr<DbusPldmEffecterStub>>&
            effectersGroup,
        testing::Sequence s = testing::Sequence())
    {
        if (pcieKnobType == PcieKnobType::PL1)
        {
            EXPECT_CALL(*effectersGroup[PcieKnobType::PL1],
                        setEffecter(limitValue))
                .InSequence(s)
                .WillOnce(throwSdBusError<bool>(std::errc::not_supported,
                                                "Error: not supported"));
            EXPECT_CALL(*effectersGroup[PcieKnobType::PL1Tau],
                        setEffecter(testing::_))
                .InSequence(s)
                .WillOnce(throwSdBusError<bool>(std::errc::not_supported,
                                                "Error: not supported"));
        }
        else if (pcieKnobType == PcieKnobType::PL2)
        {
            EXPECT_CALL(*effectersGroup[PcieKnobType::PL2],
                        setEffecter(limitValue))
                .InSequence(s)
                .WillOnce(throwSdBusError<bool>(std::errc::not_supported,
                                                "Error: not supported"));
            EXPECT_CALL(*effectersGroup[PcieKnobType::PL2Tau],
                        setEffecter(testing::_))
                .InSequence(s)
                .WillOnce(throwSdBusError<bool>(std::errc::not_supported,
                                                "Error: not supported"));
        }
    }

    void setNoLimitExpectation(
        PcieKnobType pcieKnobType,
        std::map<PcieKnobType, std::unique_ptr<DbusPldmEffecterStub>>&
            effectersGroup,
        testing::Sequence s = testing::Sequence())
    {
        if (pcieKnobType == PcieKnobType::PL1)
        {
            EXPECT_CALL(*effectersGroup[PcieKnobType::PL1],
                        setEffecter(testing::_))
                .Times(0)
                .InSequence(s);
            EXPECT_CALL(*effectersGroup[PcieKnobType::PL1Tau],
                        setEffecter(testing::_))
                .Times(0)
                .InSequence(s);
        }
        else if (pcieKnobType == PcieKnobType::PL2)
        {
            EXPECT_CALL(*effectersGroup[PcieKnobType::PL2],
                        setEffecter(testing::_))
                .Times(0)
                .InSequence(s);
            EXPECT_CALL(*effectersGroup[PcieKnobType::PL2Tau],
                        setEffecter(testing::_))
                .Times(0)
                .InSequence(s);
        }
    }

    void setPldmProviderGetTidReturnValue(std::optional<std::string> value)
    {
        ON_CALL(*pldmEntityProvider_, getTid(deviceIndex))
            .WillByDefault(testing::Return(value));
    }

    void setPldmProviderGetDeviceNameReturnValue(
        std::optional<std::string> value)
    {
        ON_CALL(*pldmEntityProvider_, getDeviceName(deviceIndex))
            .WillByDefault(testing::Return(value));
    }

    void doDiscoveryCallback()
    {
        (*storedDiscoveryDataCallback)();
    }

    std::map<PcieKnobType, std::unique_ptr<DbusPldmEffecterStub>>& getAts()
    {
        return dbusPldmEffecters_;
    }

    std::map<PcieKnobType, std::unique_ptr<DbusPldmEffecterStub>>& getPvc()
    {
        return pvcDbusPldmEffecters_;
    }

    std::shared_ptr<PcieDbusKnob> sut_;
    static constexpr auto pciePowerCapabilitiesMaxPldmValue{4000.0};

  private:
    std::shared_ptr<PldmEntityProviderMock> pldmEntityProvider_ =
        std::make_shared<testing::NiceMock<PldmEntityProviderMock>>();
    std::shared_ptr<SensorReadingsManagerMock> sensorReadingsManager_ =
        std::make_shared<testing::NiceMock<SensorReadingsManagerMock>>();
    std::shared_ptr<SensorReadingMock>
        pciePowerCapabilitiesMaxPldmSensorReading_ =
            std::make_shared<testing::NiceMock<SensorReadingMock>>();
    std::map<PcieKnobType, std::unique_ptr<DbusPldmEffecterStub>>
        dbusPldmEffecters_;
    std::map<PcieKnobType, std::unique_ptr<DbusPldmEffecterStub>>
        pvcDbusPldmEffecters_;
    DeviceIndex deviceIndex{0};
    std::shared_ptr<std::function<void(void)>> storedDiscoveryDataCallback;

    std::unique_ptr<DbusPldmEffecterStub>
        makePldmEffecter(const std::string& deviceName, const std::string& tid,
                         const DeviceIndex deviceIndex,
                         const std::string& effecterType,
                         const std::string& effecterName)
    {
        auto effecter = std::make_unique<DbusPldmEffecterStub>(
            DbusEnvironment::getIoc(), DbusEnvironment::getBus(),
            DbusEnvironment::getObjServer(), deviceName, tid, deviceIndex,
            effecterType, effecterName);
        setPLDMEffecterAvailability(effecter, true);

        return effecter;
    }

    void setPLDMEffecterAvailability(
        std::unique_ptr<DbusPldmEffecterStub>& effecter, bool available)
    {
        ON_CALL(effecter->functional, getValue())
            .WillByDefault(testing::Return(available));
    }

    bool waitForTask(
        const std::function<bool(void)>& task,
        std::chrono::milliseconds timeout = std::chrono::milliseconds(1000))
    {
        constexpr auto precission = std::chrono::milliseconds(10);
        auto elapsed = std::chrono::milliseconds(0);

        while (!task() && elapsed <= timeout)
        {
            DbusEnvironment::synchronizeIoc();
            std::this_thread::sleep_for(precission);
            elapsed += precission;
        }
        if (elapsed > timeout)
        {
            throw std::runtime_error("Timed out while waiting for future");
        }

        return true;
    }
};

TEST_F(PcieDbusKnobTest, SetKnob_ValueIsSet)
{
    setDbusSetEffecterExpectation(PcieKnobType::PL1, 100.0, getAts());
    setDbusSetEffecterExpectation(PcieKnobType::PL2, 120.0, getAts());

    sut_->setKnob(100.0);
    sut_->run();

    ASSERT_TRUE(waitForValueSet(100.0));
}

TEST_F(PcieDbusKnobTest, SetKnobTwiceTheSameValue_SetEffecterCalledOnce)
{
    testing::Sequence pl1_s, pl2_s;

    setDbusSetEffecterExpectation(PcieKnobType::PL1, 2000.0, getAts(), pl1_s);
    setDbusSetEffecterExpectation(PcieKnobType::PL2, 2400.0, getAts(), pl2_s);

    setNoLimitExpectation(PcieKnobType::PL1, getAts(), pl1_s);
    setNoLimitExpectation(PcieKnobType::PL2, getAts(), pl2_s);

    sut_->setKnob(2000.0);
    sut_->run();

    ASSERT_TRUE(waitForValueSet(2000.0));

    sut_->setKnob(2000.0);
    sut_->run();

    ASSERT_TRUE(waitForValueSet(2000.0, std::chrono::milliseconds(0)));
}

TEST_F(PcieDbusKnobTest, SetKnobTwiceDifferentValue_SetEffecterCalledTwice)
{
    testing::Sequence pl1_s, pl2_s;

    setDbusSetEffecterExpectation(PcieKnobType::PL1, 500.0, getAts(), pl1_s);
    setDbusSetEffecterExpectation(PcieKnobType::PL2, 600.0, getAts(), pl2_s);

    setDbusSetEffecterExpectation(PcieKnobType::PL1, 400.0, getAts(), pl1_s);
    setDbusSetEffecterExpectation(PcieKnobType::PL2, 480.0, getAts(), pl2_s);

    sut_->setKnob(500.0);
    sut_->run();

    ASSERT_TRUE(waitForValueSet(500.0));

    sut_->setKnob(400.0);
    sut_->run();

    ASSERT_TRUE(waitForValueSet(400.0));
}

TEST_F(PcieDbusKnobTest, ResetKnob_SetEffecterMaxValue)
{
    testing::Sequence pl1_s, pl2_s;

    setDbusSetEffecterExpectation(PcieKnobType::PL1, 100.0, getAts(), pl1_s);
    setDbusSetEffecterExpectation(PcieKnobType::PL2, 120.0, getAts(), pl2_s);

    setDbusSetEffecterExpectation(
        PcieKnobType::PL1, pciePowerCapabilitiesMaxPldmValue, getAts(), pl1_s);
    setDbusSetEffecterExpectation(
        PcieKnobType::PL2, pciePowerCapabilitiesMaxPldmValue, getAts(), pl2_s);

    sut_->setKnob(100.0);
    sut_->run();

    ASSERT_TRUE(waitForValueSet(100.0));

    sut_->resetKnob();
    sut_->run();

    ASSERT_TRUE(waitForIsKnobSet(false));
}

TEST_F(PcieDbusKnobTest, ResetKnobTwice_SetEffecterMaxValueOnce)
{
    testing::Sequence pl1_s, pl2_s;

    setDbusSetEffecterExpectation(PcieKnobType::PL1, 100.0, getAts(), pl1_s);
    setDbusSetEffecterExpectation(PcieKnobType::PL2, 120.0, getAts(), pl2_s);

    setDbusSetEffecterExpectation(
        PcieKnobType::PL1, pciePowerCapabilitiesMaxPldmValue, getAts(), pl1_s);
    setDbusSetEffecterExpectation(
        PcieKnobType::PL2, pciePowerCapabilitiesMaxPldmValue, getAts(), pl2_s);

    setNoLimitExpectation(PcieKnobType::PL1, getAts(), pl1_s);
    setNoLimitExpectation(PcieKnobType::PL2, getAts(), pl2_s);

    sut_->setKnob(100.0);
    sut_->run();

    ASSERT_TRUE(waitForValueSet(100.0));

    sut_->resetKnob();
    sut_->run();

    ASSERT_TRUE(waitForIsKnobSet(false));

    sut_->resetKnob();
    sut_->run();

    ASSERT_TRUE(waitForIsKnobSet(false, std::chrono::milliseconds(0)));
}

TEST_F(PcieDbusKnobTest, TidNotAvailable_NoLimitSet)
{
    setNoLimitExpectation(PcieKnobType::PL1, getAts());
    setNoLimitExpectation(PcieKnobType::PL2, getAts());

    setPldmProviderGetTidReturnValue(std::nullopt);

    doDiscoveryCallback();
    ASSERT_TRUE(waitForAvailability(false, false));

    sut_->setKnob(150.0);
    sut_->run();

    ASSERT_TRUE(waitForIsKnobSet(false));
}

TEST_F(PcieDbusKnobTest, DeviceNotAvailable_NoLimitSet)
{
    setNoLimitExpectation(PcieKnobType::PL1, getAts());
    setNoLimitExpectation(PcieKnobType::PL2, getAts());

    setPldmProviderGetDeviceNameReturnValue(std::nullopt);

    doDiscoveryCallback();
    ASSERT_TRUE(waitForAvailability(false, false));

    sut_->setKnob(250.0);
    sut_->run();

    ASSERT_TRUE(waitForIsKnobSet(false));
}

TEST_F(PcieDbusKnobTest, DeviceChange_SetEffecterCalled)
{
    setNoLimitExpectation(PcieKnobType::PL1, getAts());
    setNoLimitExpectation(PcieKnobType::PL2, getAts());

    setDbusSetEffecterExpectation(PcieKnobType::PL1, 100.0, getPvc());
    setDbusSetEffecterExpectation(PcieKnobType::PL2, 120.0, getPvc());

    setPldmProviderGetDeviceNameReturnValue("PVC");
    setPldmProviderGetTidReturnValue("2");

    doDiscoveryCallback();
    ASSERT_TRUE(waitForAvailability(true, true));

    sut_->setKnob(100.0);
    sut_->run();

    ASSERT_TRUE(waitForValueSet(100.0));
}

TEST_F(PcieDbusKnobTest, SetKnob_HealthIsOk)
{
    setDbusSetEffecterExpectation(PcieKnobType::PL1, 5.0, getAts());
    setDbusSetEffecterExpectation(PcieKnobType::PL2, 6.0, getAts());

    sut_->setKnob(5);
    sut_->run();

    ASSERT_TRUE(waitForValueSet(5.0));
    ASSERT_TRUE(waitForHealthSet(NmHealth::ok));
}

TEST_F(PcieDbusKnobTest, SetKnobReturnError_HealthIsWarning)
{
    setDbusSetEffecterExpectation(PcieKnobType::PL1, 5.0, getAts());
    setErrorLimitExpectation(PcieKnobType::PL2, 6.0, getAts());

    sut_->setKnob(5);
    sut_->run();

    ASSERT_TRUE(waitForHealthSet(NmHealth::warning));
}

TEST_F(PcieDbusKnobTest, InittialyHealthIsOk)
{
    sut_->run();
    EXPECT_EQ(sut_->getHealth(), NmHealth::ok);
}

TEST_F(PcieDbusKnobTest, InitiallyIsKnobSetReturnsFalse)
{
    EXPECT_FALSE(sut_->isKnobSet());
}

TEST_F(PcieDbusKnobTest, SetKnob_IsKnobSetReturnsTrue)
{
    setDbusSetEffecterExpectation(PcieKnobType::PL1, 5.0, getAts());
    setDbusSetEffecterExpectation(PcieKnobType::PL2, 6.0, getAts());

    sut_->setKnob(5);
    sut_->run();

    EXPECT_TRUE(waitForIsKnobSet(true));
}

TEST_F(PcieDbusKnobTest, SetKnobWithError_IsKnobSetReturnsFalse)
{
    testing::Sequence pl1_s, pl2_s;

    setDbusSetEffecterExpectation(PcieKnobType::PL1, 10.0, getAts(), pl1_s);
    setDbusSetEffecterExpectation(PcieKnobType::PL2, 12.0, getAts(), pl2_s);

    setDbusSetEffecterExpectation(PcieKnobType::PL1, 20.0, getAts(), pl1_s);
    setErrorLimitExpectation(PcieKnobType::PL2, 24.0, getAts(), pl2_s);

    sut_->setKnob(10.0);
    sut_->run();

    ASSERT_TRUE(waitForValueSet(10.0));

    sut_->setKnob(20.0);
    sut_->run();

    EXPECT_TRUE(waitForIsKnobSet(false));
}

TEST_F(PcieDbusKnobTest, ResetKnob_IsKnobSetReturnsFalse)
{
    testing::Sequence pl1_s, pl2_s;

    setDbusSetEffecterExpectation(PcieKnobType::PL1, 10.0, getAts(), pl1_s);
    setDbusSetEffecterExpectation(PcieKnobType::PL2, 12.0, getAts(), pl2_s);

    setDbusSetEffecterExpectation(
        PcieKnobType::PL1, pciePowerCapabilitiesMaxPldmValue, getAts(), pl1_s);
    setDbusSetEffecterExpectation(
        PcieKnobType::PL2, pciePowerCapabilitiesMaxPldmValue, getAts(), pl2_s);

    sut_->setKnob(10.0);
    sut_->run();

    ASSERT_TRUE(waitForValueSet(10.0));

    sut_->resetKnob();
    sut_->run();

    EXPECT_TRUE(waitForIsKnobSet(false));
}

} // namespace nodemanager