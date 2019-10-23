# MESOscope
Code in C++ for controlling the NI USB-7852R card

## To do:
### C++
Caution on:
- In Mesoscope::openShutter(), I commented out VirtualLaser::isLaserInternalShutterOpen() to check that the laser has the shutter open because it is not needed when running the full sequence and any comm error will stop the routine
- Keep an eye on the Z-stage bouncing that triggers the ctl&acq
Sequencer:
- The last time I imaged liver, I noticed that stepwise and contZ scanning were vertically shifted wrt each other. I thought it was because I used pixelSizeZ = 1.0 um instead of 0.5 um,
and therefore, the timing has to be re-calibrated for 1.0 um. However, I checked the bead position for stepwise and contZ (upward and downward) and the match perfectly.
- Choose 1X or 16X dynamically
- Enforce to close the Uniblitz shutter before cutting
- Updated the Z position after cutting --> check that it works
- enable/disable using the vibratome in Routines::sequencer
Post-processing
- Implement suppressCrosstalk() flattenField() on the GPU
- Multithread demuxAllChannels_()
Others:
- Do a post-sequence clean up routine to set the pockels outputs to 0
- Maybe switch to smart pointers for the data. Check the overhead
- For the vibratome, show a progress bar for the slicing sequence
- Maybe install VTK to display tiff images

### LabView
- I detected that shutter #2 is triggered when the FPGA resets at the end of the code. I see a small voltage on the scope, like ~50mV that seems to be enough to trigger the shutter

### Hardware
- realign the dichroic and rescanner mounts. Fine tune the galvo parameters
- Screw down the PMT16X
- Test cooling down the PMT16X


### Things that could be improved in the future
- Mount the collector lens on the detector platform
- Use a motorized stage for the PMT16X mount
- Use an apodizing filter to make the fluorescence emission even for the different beamlets
- Use a grid mask on the PMT16X to reduce the crosstalk
- Use motorized actuator on the mirrors that align the Ti-Sapphire laser to the microscope because the laser beam slightly wanders with the wavelength tuning
- Set up the beads next to the sample holder permanently
- Purchase a 5W power pockels (instead of 10W) for Fidelity to have a finer control
- Mount the bead and fluorescent slides in the sample container for checking the microscope performance regularly
- Improve the dichroic mount to allow fine rotation
- The oil in the sample container crawls up through the clamps
- Add motorized actuators to the last corner mirror before the galvo scanner