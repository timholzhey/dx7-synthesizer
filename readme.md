# FM Synthesizer inspired by Yamaha DX7 - Reference software implementation for FPGA project

This repository holds the software reference implementation for an FPGA project to recreate the Yamaha DX7 synthesizer and generates lookup tables to be used in the Verilog code.
Find all Verilog code here: https://git.tu-berlin.de/timholzhey/soc-riscv-synth

## Install portaudio

```bash
cd libs/portaudio && ./configure && make && sudo make install
```

## Demo
![Demo](https://github.com/timholzhey/dx7-synthesizer/assets/39774057/4269f4d6-d485-45bb-8cfa-d7995d7436f8)
