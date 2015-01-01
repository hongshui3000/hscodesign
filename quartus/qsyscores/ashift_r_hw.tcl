# TCL File Generated by Component Editor 14.0
# Tue Dec 30 19:43:17 CET 2014
# DO NOT MODIFY


#
# ashift_r "ashift_r" v1.0
#  2014.12.30.19:43:17
#
#

#
# request TCL package from ACDS 14.0
#
package require -exact qsys 13.1


#
# module ashift_r
#
set_module_property DESCRIPTION ""
set_module_property NAME ashift_r
set_module_property VERSION 1.0
set_module_property INTERNAL false
set_module_property OPAQUE_ADDRESS_MAP true
set_module_property AUTHOR ""
set_module_property DISPLAY_NAME ashift_r
set_module_property INSTANTIATE_IN_SYSTEM_MODULE true
set_module_property EDITABLE true
set_module_property ANALYZE_HDL AUTO
set_module_property REPORT_TO_TALKBACK false
set_module_property ALLOW_GREYBOX_GENERATION false


#
# file sets
#
add_fileset QUARTUS_SYNTH QUARTUS_SYNTH "" ""
set_fileset_property QUARTUS_SYNTH TOP_LEVEL ashift_r
set_fileset_property QUARTUS_SYNTH ENABLE_RELATIVE_INCLUDE_PATHS false
add_fileset_file "ashift_r .vhd" VHDL PATH "../../vhdl/instructions/ashift_r .vhd" TOP_LEVEL_FILE


#
# parameters
#


#
# display items
#


#
# connection point nios_custom_instruction_slave
#
add_interface nios_custom_instruction_slave nios_custom_instruction end
set_interface_property nios_custom_instruction_slave clockCycle 0
set_interface_property nios_custom_instruction_slave operands 2
set_interface_property nios_custom_instruction_slave ENABLED true
set_interface_property nios_custom_instruction_slave EXPORT_OF ""
set_interface_property nios_custom_instruction_slave PORT_NAME_MAP ""
set_interface_property nios_custom_instruction_slave CMSIS_SVD_VARIABLES ""
set_interface_property nios_custom_instruction_slave SVD_ADDRESS_GROUP ""

add_interface_port nios_custom_instruction_slave a dataa Input 32
add_interface_port nios_custom_instruction_slave b datab Input 32
add_interface_port nios_custom_instruction_slave r result Output 32
