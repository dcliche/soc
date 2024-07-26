SITE_MK ?= ../../site.mk

-include $(SITE_MK)

YOSYS := $(OSS_CAD_SUITE_PATH)yosys
ECPPACK := $(OSS_CAD_SUITE_PATH)ecppack
NEXTPNR := $(OSS_CAD_SUITE_PATH)nextpnr-ecp5
FUJPROG := $(OSS_CAD_SUITE_PATH)fujprog

DEVICE := 85k
PACKAGE := CABGA381

PIN_DEF := ulx3s.lpf
SRC_CORE := ../../external/xglib/rtl/riscv/processor.sv \
		../../external/xglib/rtl/riscv/alu.sv \
		../../external/xglib/rtl/riscv/decoder.sv \
		../../external/xglib/rtl/riscv/register_file.sv \
		../../external/xglib/rtl/riscv/multiplier.v \
		../../external/xglib/rtl/riscv/divider.v \
		../../external/xglib/rtl/sdram_ctrl.v \
		../../external/xglib/rtl/cache_ctrl.v \
		../../external/xglib/rtl/display_timings.sv \
		../../external/jtopl/hdl/jtopl_acc.v \
		../../external/jtopl/hdl/jtopl_csr.v \
		../../external/jtopl/hdl/jtopl_div.v \
		../../external/jtopl/hdl/jtopl_eg.v \
		../../external/jtopl/hdl/jtopl_eg_cnt.v \
		../../external/jtopl/hdl/jtopl_eg_comb.v \
		../../external/jtopl/hdl/jtopl_eg_ctrl.v \
		../../external/jtopl/hdl/jtopl_eg_final.v \
		../../external/jtopl/hdl/jtopl_eg_pure.v \
		../../external/jtopl/hdl/jtopl_eg_step.v \
		../../external/jtopl/hdl/jtopl_exprom.v \
		../../external/jtopl/hdl/jtopl_lfo.v \
		../../external/jtopl/hdl/jtopl_logsin.v \
		../../external/jtopl/hdl/jtopl_noise.v \
		../../external/jtopl/hdl/jtopl_op.v \
		../../external/jtopl/hdl/jtopl_pg.v \
		../../external/jtopl/hdl/jtopl_pg_comb.v \
		../../external/jtopl/hdl/jtopl_pg_inc.v \
		../../external/jtopl/hdl/jtopl_pg_rhy.v \
		../../external/jtopl/hdl/jtopl_pg_sum.v \
		../../external/jtopl/hdl/jtopl_pm.v \
		../../external/jtopl/hdl/jtopl_reg_ch.v \
		../../external/jtopl/hdl/jtopl_sh.v \
		../../external/jtopl/hdl/jtopl_sh_rst.v \
		../../external/jtopl/hdl/jtopl_single_acc.v \
		../../external/jtopl/hdl/jtopl_slot_cnt.v \
		../../external/jtopl/hdl/jtopl_timers.v \
		../../external/jtopl/hdl/jtopl.v \
		../../external/jtopl/hdl/jtopl2.v \
		../../external/jtopl/hdl/jtopl_mmr.v \
		../../external/jtopl/hdl/jtopl_reg.v \
		../dac.v \
		../sdram.sv \
		../bram.sv \
		../uart.sv \
		../xga.sv \
		../vga.sv \
		../fifo.sv \
		../ps2kbd.v \
		../ps2mouse.v \
		../hdmi_encoder.v \
		../xgsoc.sv \
		../usb/usb_phy.v \
		../usb/usb_rx_phy.v \
		../usb/usb_tx_phy.v \
		../usb/usbh_crc5.v \
		../usb/usbh_crc16.v \
		../usb/usbh_host_hid.v \
		../usb/usbh_sie.v \
		generated_pll_main.v \
		generated_pll_video.v \
		generated_pll_usb.v \
		top.sv

SRC_XOSERA := $(wildcard ../../external/Xosera/rtl/*.sv)

#SRC := $(SRC_CORE) $(SRC_XOSERA)
SRC := $(SRC_CORE)

#DEFINES := -DUSB -DPS2 -DSDRAM -DSD_CARD -DFLASH -DXGA -DNO_TESTPATTERN -DEN_PF_B -DEN_PF_B_BLEND -DEN_AUDIO=4 -DMODE_848x480
DEFINES := -DUSB -DSDRAM -DVGA -DOPL

all: top.bin

$(SITE_MK):
	$(info Copy the example site.template file to site.mk and edit the paths.)
	$(error site.mk not found.)

clean:
	rm -f *.hex *.asc *.json *.bin *.log

top.json: $(SRC) ../../firmware/firmware.hex
	cp ../../firmware/firmware.hex .
	$(YOSYS) -ql top.log -p 'verilog_defines $(DEFINES) ; read_verilog -sv $(SRC); synth_ecp5 -top top -json top.json'

top.asc: top.json $(PIN_DEF)
	$(NEXTPNR) -l top_nextpnr.log --$(DEVICE) --package $(PACKAGE) --json top.json --lpf $(PIN_DEF) --textcfg top.asc --randomize-seed

top.bin: top.asc
	$(ECPPACK) --compress --input top.asc --bit top.bin

prog: top.bin
	$(FUJPROG) -T bit top.bin

.PHONY: all prog
