# Copyright (c) 2026 ETH Zurich and University of Bologna.
# Licensed under the Apache License, Version 2.0, see LICENSE for details.
# SPDX-License-Identifier: Apache-2.0
#

if {[info script] ne ""} {
    set sta_dir [file normalize [file dirname [info script]]/..]
    set proj_dir [file normalize ${sta_dir}/..]
    cd $sta_dir
} else {
    set sta_dir [pwd]
    set proj_dir [file normalize ${sta_dir}/..]
}

file mkdir reports

set top_design croc_chip
set netlist [file join $proj_dir yosys out croc_yosys.v]
set report_file [file join $sta_dir reports sta.rpt]

if {[file exists [file join $proj_dir technology]]} {
    set pdk_dir [file join $proj_dir technology]
    set pdk_cells_lib $pdk_dir/lib
    set pdk_sram_lib  $pdk_dir/lib
    set pdk_io_lib    $pdk_dir/lib
} else {
    set pdk_dir [file join $proj_dir ihp13 pdk]
    set pdk_cells_lib [file join $pdk_dir ihp-sg13g2 libs.ref sg13g2_stdcell lib]
    set pdk_sram_lib  [file join $pdk_dir ihp-sg13g2 libs.ref sg13g2_sram lib]
    set pdk_io_lib    [file join $pdk_dir ihp-sg13g2 libs.ref sg13g2_io lib]
}

read_liberty [file join $pdk_cells_lib sg13g2_stdcell_typ_1p20V_25C.lib]
read_liberty [file join $pdk_io_lib sg13g2_io_typ_1p2V_3p3V_25C.lib]
foreach file [glob -nocomplain -directory $pdk_sram_lib *_typ_1p20V_25C.lib] {
    read_liberty $file
}

read_verilog $netlist
link_design $top_design

set old_pwd [pwd]
cd [file join $proj_dir openroad]
read_sdc src/constraints.sdc
cd $old_pwd

report_checks -path_delay max -fields {slew cap input nets fanout} -format full_clock_expanded > $report_file
report_worst_slack >> $report_file
report_tns >> $report_file
report_clock_skew >> $report_file
report_checks -unconstrained -format end -no_line_splits >> $report_file
