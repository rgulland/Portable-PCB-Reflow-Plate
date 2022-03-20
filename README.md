# Portable PCB Reflow Plate
A PCB reflow plate optimized for medium size boards.  
Design, assembly & testing documentation at https://rgulland.com/blog/2022/3/11/reflow-heater

# Build Notes
Please don't build this unless you're confident working with mains power and have sufficient materials to do it safely. This build is rather fiddly and time consuming: if you're unsure I'm considering making a v2 that comes as a kit with custom electronics, contact me if you're interested in that.  
On initial power on, make sure to set a default power limit of 125-150W & 30.1V on the DSP5005 when using the AC input. Above these limits, the AC-DC converter components will have a shorter life due to high temperatures.

# LICENSE
This project is under the GPL v3 license. There's little to no IP here, as most components are COTS, so this is more an endorsement of keeping more things open source. Open source hardware projects are rare, so if yours is, thanks :)  
https://www.gnu.org/licenses/gpl-3.0.en.html

# Changes Since Blog
- Adjusted microcontroller port cutout
- Added draft to bottom cap to better tolerate 3D printed parts' first layer flange / 'elephant's foot'
- Separated code into multiple files & commented for readability

# Todo
- Improve code
  - Generally remove jank / simplify screen update system
  - Re-implement PID control
  - Add thermal runaway detection / better AC lockout
  - PID settings / tuning menu
  - Add curve edit mode
- Design / Build / Test v2 
  - SD card / spi flash / other permanent storage for reflow curves
  - Cheaper thermal insulation solution
  - Lower cost MCU (ideally w/ BLE or wifi for future expandability)
  - Wiring condensed into 3 custom PCBs to reduce assembly time
  - Smaller AC port (fused IEC 60320) & SPST switch
  - Larger cooling fan
  - Solution for cooling PCBs faster
  - SSR for finer PID control
