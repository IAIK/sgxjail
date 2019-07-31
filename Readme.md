# SGXJail

This repository contains the SGXJail code. 
SGXJail is a defense mechanism against malicious enclaves attempting to 
hijack their hosting application. 
The corresponding academic paper has been published as:

Samuel Weiser, Luca Mayr, Michael Schwarz, Daniel Gruß. 
**SGXJail: Defeating Enclave Malware via Confinement**. 
In *Research in Attacks, Intrusions and Defenses* 2019.


## Repository Structure

| Folder        | Description                                                            |
|---------------|------------------------------------------------------------------------|
| `linux-sgx`   | The normal Linux SGX SDK version 2.4                                   |
| `sgxjail-sdk` | The SGXJail SDK                                                        |
| `SampleCode`  | Examples for the normal SDK (verify_vanilla) and SGXJail (verify_sbox) |

## Setup

Initialize the repository with

 `git submodule update --init --recursive`

Install the normal Linux SGX SDK in `/opt/intel` by following this [README.md](https://github.com/intel/linux-sgx/blob/sgx_2.4/README.md)

To build a sample project, navigate to `SampleCode/verify_xxx` and run:

`make SGX_MODE=HW SGX_PRERELEASE=1`

Now you can run the application via `./app`

## Disclaimer

This is a research prototype without any guarantees. Use it at your own risk!

## Known limitations

* Multiple enclaves per process are not supported
* Multithreading is not supported
* Nested OCALLs are not supported

See the paper for more limitations.
