#!/bin/sh

echo The purnell press power button , AC will OFF

# sysadmin@purnell:~# i2ctransfer -f -y 24 w4@0x5d 0x11 0x1b 0x00 0x00 r1
# 0x00
# The PDB MCU_PA27_PDB_BD_00_HOTSWAP_EN = LOW , AC Power is OFF. (*1)
# Why I2C address is 0x5d , refer to PDB spec (*2)
# PDB_BD_ID0 and PDB_BD_ID1 is SGPIO (*3)

ID0=$(gpioget `gpiofind PDB_BD_ID0`)
ID1=$(gpioget `gpiofind PDB_BD_ID1`)

if [ "$ID0" == "0" ] && [ "$ID1" == "0" ]; then
	echo "The NODE is 0 , The PDB MCU_PA27_PDB_BD_00_HOTSWAP_EN = LOW , It will be AC OFF"
	i2ctransfer -f -y 27 w4@0x5d 0x11 0x1b 0x00 0x00 r1
fi

if [ "$ID0" == "1" ] && [ "$ID1" == "0" ]; then
	echo "The NODE is 1 , The PDB MCU_PA28_PDB_BD_01_HOTSWAP_EN = LOW , It will be AC OFF"
    	i2ctransfer -f -y 27 w4@0x5d 0x11 0x1c 0x00 0x00 r1
fi

if [ "$ID0" == "0" ] && [ "$ID1" == "1" ]; then
	echo "The NODE is 2 , The PDB MCU_PB22_PDB_BD_10_HOTSWAP_EN = LOW , It will be AC OFF"
	i2ctransfer -f -y 27 w4@0x5d 0x11 0x36 0x00 0x00 r1
fi

if [ "$ID0" == "1" ] && [ "$ID1" == "1" ]; then
	echo "The NODE is 3 , The PDB MCU_PB23_PDB_BD_11_HOTSWAP_EN = LOW , It will be AC OFF"
	i2ctransfer -f -y 27 w4@0x5d 0x11 0x37 0x00 0x00 r1
fi

# Remark:
# (*1) 	PDB Schematic : INTEL_BHS_2U4N_PDB_20230531_decrypted.pdf
#    	http://tao-pdmnet-4:8080/tfs/TAO_BU5_FW5/FW5E/_git/Purnell-DOCS/commit/e31895942d51449846ab8af1d95d013f04723ae5?refName=refs%2Fheads%2Fdocs&path=%2FHW_Schematic%2FPurnell_Schematic%2FPDB%2FINTEL_BHS_2U4N_PDB_20230531_decrypted.pdf&_a=contents

# (*2)	PDB FW Spec : Purnell_PDB_PIC_FW_Spec_V0.07.docx page 14 , SET_GPIO_STATUS Command
#	http://tao-pdmnet-4:8080/tfs/TAO_BU5_FW5/FW5E/_git/Purnell-DOCS/commit/635c110da2e62cc45b46d190c43c205b642176fc?refName=refs%2Fheads%2Fdocs&path=%2FPurnell_Project_Information%2FPDB%2FPurnell_PDB_PIC_FW_Spec_V0.07.docx&_a=compare	

# (*3) SGPIO Table : Purnell_SGPIO_Mapping_20231108.xlsx
#	http://tao-pdmnet-4:8080/tfs/TAO_BU5_FW5/FW5E/_git/Purnell-DOCS/commit/70ed010483dd00065ee465d603bbdf13f4f28792?refName=refs%2Fheads%2Fdocs&path=%2FPurnell_Project_Information%2FCPLD%2FPurnell_SGPIO_Mapping_20231124.xlsx&_a=contents
