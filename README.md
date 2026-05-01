# Internet Speed Test (C Project)

This project is a command-line application written in C that measures internet download and upload speeds using real servers.

## Features

* Detects user location using an online API
* Selects the best available server automatically
* Performs download speed test (max 15 seconds)
* Performs upload speed test (max 15 seconds)
* Supports individual and automatic testing modes
* Uses libcurl, getopt, and cJSON libraries

## Technologies Used

* C Programming Language
* libcurl (HTTP requests)
* cJSON (JSON parsing)
* getopt (command-line arguments)

## Build Instructions

```bash
make
```

## Run Instructions

```bash
./speedtest --auto
./speedtest --download
./speedtest --upload
```

## Output

* Download speed in Mbps
* Upload speed in Mbps
* Selected server
* User location

## Notes

* Server list is loaded from `speedtest_server_list.json.`
* Some servers may not respond
* Each test is limited to 15 seconds

## Author

Gul Jan
