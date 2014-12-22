# TCL File Generated by Component Editor 13.1
# Mon Dec 22 16:30:23 CET 2014
# DO NOT MODIFY


# 
# bit_extract_1 "bit_extract_1" v1.0
# Michael Platzer, Thomas Weber and Florian Zaruba 2014.12.22.16:30:23
# Extracts upper 32 bit of a word.
# 

# 
# request TCL package from ACDS 13.1
# 
package require -exact qsys 13.1


# 
# module bit_extract_1
# 
set_module_property DESCRIPTION "Extracts upper 32 bit of a word."
set_module_property NAME bit_extract_1
set_module_property VERSION 1.0
set_module_property INTERNAL false
set_module_property OPAQUE_ADDRESS_MAP true
set_module_property AUTHOR "Michael Platzer, Thomas Weber and Florian Zaruba"
set_module_property DISPLAY_NAME bit_extract_1
set_module_property INSTANTIATE_IN_SYSTEM_MODULE true
set_module_property EDITABLE true
set_module_property ANALYZE_HDL AUTO
set_module_property REPORT_TO_TALKBACK false
set_module_property ALLOW_GREYBOX_GENERATION false


# 
# file sets
# 
add_fileset QUARTUS_SYNTH QUARTUS_SYNTH "" ""
set_fileset_property QUARTUS_SYNTH TOP_LEVEL extr_bits_1
set_fileset_property QUARTUS_SYNTH ENABLE_RELATIVE_INCLUDE_PATHS false
add_fileset_file extr_bits_1.vhd VHDL PATH extr_bits_1.vhd TOP_LEVEL_FILE


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
add_interface_port nios_custom_instruction_slave pn datab Input 32
add_interface_port nios_custom_instruction_slave r result Output 32
