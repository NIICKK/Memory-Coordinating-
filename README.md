# Memory-Coordinating-
Memory coordinator to dynamically manage the memory resources assigned to each guest machine.
Compile

Just use command "make" in terminal. Make sure libvirt/libvirt.h is installed properly in include folder.

Usage

./Memory_coordinator [time_interval] 

eg ./Memory_coordinator 12

Algorithm

The memory coordinator allocates memory dynamically and make sure that in each domain, the free memory percentage is between 30% to 60% (or you can set other threshold)

For each domain:
	if domain has free memory > 60%, then the host gets back half of its free memory
	
	if domain has free memory < 30%,
		if the host has a lot free memory (>50%), give domain a lot free memory (300mb)
		if the host has <50% free memory, give domain some free memory(200mb)

In this way, the memory in each domain is balanced in 30~60 percent range

