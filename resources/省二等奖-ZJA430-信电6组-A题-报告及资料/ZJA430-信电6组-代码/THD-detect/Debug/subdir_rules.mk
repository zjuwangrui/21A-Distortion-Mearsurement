################################################################################
# Automatically-generated file. Do not edit!
################################################################################

SHELL = cmd.exe

# Each subdirectory must supply rules for building sources it contributes
%.obj: ../%.c $(GEN_OPTS) | $(GEN_FILES) $(GEN_MISC_FILES)
	@echo 'Building file: "$<"'
	@echo 'Invoking: Arm Compiler'
	"C:/ti/ccs1100/ccs/tools/compiler/ti-cgt-arm_20.2.5.LTS/bin/armcl" -mv7M4 --code_state=16 --float_support=FPv4SPD16 -me --include_path="C:/ti/simplelink_msp432e4_sdk_4_20_00_12/source/third_party/CMSIS/DSP_Lib/Include" --include_path="C:/Users/DoctorX/workspace_v11/THD-detect" --include_path="C:/Users/DoctorX/workspace_v11/THD-detect/adc" --include_path="C:/Users/DoctorX/workspace_v11/THD-detect/fft" --include_path="C:/Users/DoctorX/workspace_v11/THD-detect/printf" --include_path="C:/Users/DoctorX/workspace_v11/THD-detect/sdwa" --include_path="C:/ti/simplelink_msp432e4_sdk_4_20_00_12/source/third_party/CMSIS/Include" --include_path="C:/Users/DoctorX/workspace_v11/THD-detect/goertzel" --include_path="C:/ti/simplelink_msp432e4_sdk_4_20_00_12/source/third_party/CMSIS/DSP_Lib/Include" --include_path="C:/ti/simplelink_msp432e4_sdk_4_20_00_12/source/third_party/CMSIS/DSP_Lib/PrivateInclude" --include_path="C:/Users/DoctorX/workspace_v11/THD-detect/esp8266" --include_path="C:/ti/simplelink_msp432e4_sdk_4_20_00_12/source" --include_path="C:/ti/simplelink_msp432e4_sdk_4_20_00_12/source/third_party/CMSIS/Include" --include_path="C:/ti/simplelink_msp432e4_sdk_4_20_00_12/source/ti/net/bsd" --include_path="C:/ti/ccs1100/ccs/tools/compiler/ti-cgt-arm_20.2.5.LTS/include" --define=DeviceFamily_MSP432E4 --define=__MSP432E401Y__ -g --diag_warning=225 --diag_warning=255 --diag_wrap=off --display_error_number --gen_func_subsections=on --preproc_with_compile --preproc_dependency="$(basename $(<F)).d_raw" --include_path="C:/Users/DoctorX/workspace_v11/THD-detect/Debug/syscfg" $(GEN_OPTS__FLAG) "$<"
	@echo 'Finished building: "$<"'
	@echo ' '


