# PMT pulse distribution script for digitizer data output
# by: Julie He <juhe@ucdavis.edu>

import numpy as np
import matplotlib.pyplot as plt
import sys

FILE = sys.argv[1]
SAVEFILENAME = 'my_charges.txt'
#HEADER = 8 # 7 for DT5730, 8 for DT5742

plot_waveforms = False
if len(sys.argv) == 3:
  plot_waveforms = sys.argv[2]

PEAK_VOLTAGE = 0.010 # V ; need to play around with
IMPEDANCE = 50. # Ohms
dT = 2E-10 # s ; sampling interval ; 2E-9 for DT5730

# check if file name given 
if (len(sys.argv) < 2) or (len(sys.argv) > 3):
  print("USAGE: python plot_wavetxt.py [wave0.txt] [plot_waveforms=false]")
  sys.exit(1)


def main():
  # open data file
  print("Note: ADC to V conversion is hard-coded...\nReading file...")
  with open(FILE) as f:
    data = f.readlines()
#    data = [line.split(' ')[-1] for line in f] # reads last column of values into list

  header = 0
  recordLength = 0 

  for line in data[:10]:
    # get num of samples per frame
    if (line[:6] == 'Record'):
      recordLength = int(line.split(' ')[-1])
    # get num of header lines
    try:
      dummy = float(line)
    except ValueError:
      header += 1

  print("Record Length: " + str(recordLength) + "\nNumber of header lines: " + str(header))

  print("Integrating pulses...")
  holderArray = []
  charge = []
  voltage = 0.0
  frameCounter = 0

  j = header
  while j < len(data): # while we are not at the end of file
    # convert ADC to volts and store in temp list
    # holderArray.append((float(data[j]) - (2**14)*0.86)/8192.) # V; 0.86 hard coded, look in config file

    # find avg pedestal value and subtract before adding to holderArray
    frameStart = frameCounter*(recordLength + header) + header
    if ( j == frameStart ): # so this is only done once per frame
      ini_ped = 0.
#      print(j)
      for val in data[frameStart:frameStart + recordLength/10]: # use first 10%
        ini_ped += float(val)
#      print(ini_ped)

    holderArray.append((float(data[j]) - ini_ped/len(data[frameStart:frameStart + recordLength/10]))/8192.) # 8192 hard-coded conversion from ADC to V; depends on digitizer res
    j += 1 # increment sample counter

    if (j)%(recordLength + header) == 0: # if at end of frame
#      voltage = sum(holderArray) # sum up area of entire frame
      voltage = sum(holderArray[:recordLength/2]) # sum up half the frame

      skip_frame = False # initialize skip_frame variable
      frameCounter += 1 

      # sum pedestal over first 5% of frame
      pedestal = 0.
      DC = 0.
      for k in range(len(holderArray[:recordLength/2])/20): # 20% of half frame
        # set relative DC level
        DC = (float(DC*k) + holderArray[k])/float(k + 1)
        # check for early pulses
        if abs(float(holderArray[k+1]) - DC) > PEAK_VOLTAGE: 
          skip_frame == True # skip entire frame
        else: # if no peaks, add to pedestal
          pedestal += holderArray[k]
      pedestal *= len(holderArray[:recordLength/2])/k # fix
#      print("k = " + str(k))
#      print("length of holderArray: " + str(len(holderArray)))

      if skip_frame == True:
        print("Skipped frame: " + str(frameCounter))
      else:
#        charge.append(-(voltage - pedestal)*dT/IMPEDANCE)
        charge.append(-(voltage)*dT/IMPEDANCE)

      if plot_waveforms == True:
        # plot event
        plt.ylim((-30E-3, 5E-3)) # V
        plt.title('Event ' + str(frameCounter) + ', V=' + str(voltage) + ', q=' + str(-voltage*dT/IMPEDANCE))
        plt.xlabel('sample')
        plt.ylabel('voltage (V)')
        plt.plot(holderArray)
        plt.show()

      # clear for next event
      holderArray = []
      voltage = 0.0

      j += header # skip header info
#      print("j = " + str(j))

  # check num of events
#  if (frameCounter == len(data)/(recordLength+7):
#    print("Accounted for all events")
#  else:
#    print("Did not account for all events: " + str(len(data)/1037. - frameCounter))

  print("Saving file as 'my_charges.txt'")
  np.savetxt(('{0}').format(SAVEFILENAME), charge)
#  np.savetxt('test_charges.txt', test_charge) # split up frame in 8 parts
  print("Events: " + str(frameCounter))
  return

if __name__ == '__main__':
  main()
