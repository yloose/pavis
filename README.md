# Pavis

Pavis can cast your local pulseaudio stream to a [WLED](https://github.com/Aircoookie/WLED) device in the network using fourier transformation. You can create you own algorithms which are linked and used dynamically to translate the audio to light data.


## Installation

Pavis currently only supports Linux-based systems.

### Dependencies

Dependencies needed:
  * fftw3
  * libconfig
  * libpulse

And for building and compiling:
  * cmake
  * make
  * gcc
  * git

Install on ubuntu with:

    # apt install libfftw3-dev libconfig-dev libpulse-dev cmake build-essential git

Install on arch based distros with:

    # pacman -S fftw libconfig libpulse base-devel git

### Download & Build

Clone the repo, build and install:

    git clone https://github.com/yloose/pavis.git --recursive
    cd pavis
	mkdir build && cd build
	cmake ..
	make
	sudo make install

## Usage

### Config file

The default config file lies in `/home/${USER}/.config/pavis/pavis.conf` and is construted like the following, adjust it to your needs.

```
input: {
# 	Your pulse audio input device
#	Get your default output sink with $ pactl get-default-sink
#	and append ".monitor" to use in this setting.
#	You can also ue a recording sink.
#	Leave this empty to use the default sink.
	input_device_name: "alsa_output.pci-0000_0e_00.1.analog-stereo.monitor",
 }

output: {
	# The timeout specifies how often packets are sent to the WLED devices (in microseconds)
 	packet_timeout: 50000,
	wled_devices: (
		{
			# Name of the device used to control it
			name: "Desk",
			# Number of the leds passed on to the algorithm. Currently only a maximun number of 170 are supported
			leds: 104,
			# IP address of the device
			ip: "192.168.2.100",
			# Port, default is 5568
			port: 5568,
		},
		# Second example device
		{
			name: "Couch",
			leds: 50,
			ip: "192.168.2.101",
			port: 5568,
		}

	),
}
```

### Setting up algorithms

Algorithms are stored in `/home/${USER}/.config/pavis/algorithms`.
You can copy some examples ones and helper scripts to compile them by issuing the following commands:

    mkdir -p ~/.config/pavis/algorithms && cd ~/.config/pavis
	cp /usr/share/doc/pavis/algorithms/* ./algorithms
	cp /usr/share/doc/pavis/compile_algorithms.sh ./

To compile your algorithms, either for the first time or after editing them, you can simply call the script: ``bash compile_algorithms.sh``

### Controlling the daemon

Pavis listens on a socket at `/tmp/pavis.sock` and as such can be controlled with any program that can write to it.
Here are some examples to manipulate devices with socat:

Pause or resume a client by its name as specified in the config file:

    echo ':pause <device_name>' | socat - /tmp/pavis.sock
    echo ':resume <device_name>' | socat - /tmp/pavis.sock

Select a new algorithm for a device by its file name:

    echo ':select <algorithm_name> <device_name>' | socat - /tmp/pavis.sock
 
Rescan for new algorithms:
    
	echo ':rescan' | socat - /tmp/pavis.sock 

When running pavis for the first time or after adding a new device you need to select a algorithm and use the ``:resume`` command to start streaming to the device.

A more simple cli tool is planned in the future.

### Running and logging

You can run pavis by typing ``pavis-daemon``, or you can start it (on login) via systemd with:

    sudo systemctl start pavis-daemon
	sudo systemctl enable pavis-daemon

## Developing and compiling new algorithms

Algorithms are stored in `/home/${USER}/.config/pavis/algorithms`. They are compiled as shared libraries for pavis to load at runtime.
An example algorithm and a [template](/res/algorithms/simple.template.c) for new algorithms come preinstalled and further requirements are given in the template.

To compile them use the following command (add libraries you need):

    gcc -shared -o <name_of_the_algorithm>.so -fPIC <name_of_the_source_file>
