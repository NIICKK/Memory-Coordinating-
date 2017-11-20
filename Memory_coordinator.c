#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <libvirt/libvirt.h>
#include <assert.h>

static const int THRESHOLD100 = 100 * 1024;	//100MB
static const int THRESHOLD200 = 200 * 1024;	//200MB
static const int THRESHOLD300 = 300 * 1024;	//300MB

//Main - entry point
int main(int argc, char **argv) {
  //check program argument [GOOD]
  if(argc != 2){
  	printf("Need program argument: time interval\n");
	exit(1);
  }
  int timeInterval = atoi(argv[1]);

  //declare variables [GOOD]
  int i;
  //int starveCount;
  //long memoryCollect;
  //int max,min;
  //long maxMem,minMem,memoryCollect;
  //long minMemory,maxMemory;
  int maxDomains,nparams;
  virConnectPtr conn;  
  virDomainPtr* domains = NULL;
  virNodeMemoryStatsPtr params;
  virDomainMemoryStatStruct memStats[VIR_DOMAIN_MEMORY_STAT_NR];
  long* memoryAvailable = NULL;
  long* memoryTotal = NULL;
  //connect to Hypervisor [GOOD]
  if((conn = virConnectOpen("qemu:///system")) == NULL){
  	printf("Can't connect to Hypervisor\n");
  	exit(1);
  }
  assert(conn != NULL);

  printf("Connected\n");

  //while there are active running virtual machines within "qemu:///system"
  while((maxDomains = virConnectListAllDomains(conn,&domains,VIR_CONNECT_LIST_DOMAINS_ACTIVE | VIR_CONNECT_LIST_DOMAINS_RUNNING)) > 0){	
	//collect memory stats for host machine
	nparams = 0;
	if (virNodeGetMemoryStats(conn, VIR_NODE_MEMORY_STATS_ALL_CELLS,NULL,&nparams,0) == 0 && nparams != 0){
		if ((params = malloc(sizeof(virNodeMemoryStats) * nparams)) == NULL){
			printf("Can not allocate memory to host memory stats\n");
			exit(1);		
		}
		 memset(params, 0, sizeof(virNodeMemoryStats) * nparams);
		if (virNodeGetMemoryStats(conn, VIR_NODE_MEMORY_STATS_ALL_CELLS,params,&nparams, 0)){
			printf("Can not collect memory stats from host\n");
			exit(1);		
		}
	} 
	for (i = 0; i < nparams; i++) {
		printf("%8s : %lld MB\n",params[i].field,params[i].value/1024);
	}
	//for each domain collect memory stats
	//min = max = -1;
        memoryTotal = (long*)calloc(maxDomains,sizeof(long));
	memoryAvailable = (long*)calloc(maxDomains,sizeof(long));	
	for(i=0; i<maxDomains; i++){
		if(virDomainSetMemoryStatsPeriod(domains[i],1,VIR_DOMAIN_AFFECT_CURRENT)<0){
			printf("Can not change the domain memory balloon driver\n");
			exit(1);		
		}
		
		if(virDomainMemoryStats(domains[i], memStats,VIR_DOMAIN_MEMORY_STAT_NR, 0)<0){
			printf("Can not collect memory stats\n");
			exit(1);		
		}
		memoryTotal[i] = memStats[VIR_DOMAIN_MEMORY_STAT_ACTUAL_BALLOON].val;
		memoryAvailable[i] = memStats[VIR_DOMAIN_MEMORY_STAT_AVAILABLE].val;
		//if(min == -1 || memoryAvailable[i]<minMem){
		//	min = i;minMem = memoryAvailable[i];		
		//}
		//if(max == -1 || memoryAvailable[i]>maxMem){
		//	max = i;maxMem = memoryAvailable[i];		
		//}
		printf("domain %d, memory total: %ld MB\n",i, memoryTotal[i]/1024);
		printf("domain %d, memory available: %ld MB\n",i, memoryAvailable[i]/1024);
		
	}
	//Coordianator Algorithm HERE - assign memory from max vm to min vm
	for(i=0;i<maxDomains;i++){
		//check if free memory is wasting, reduce it by 50%
		if((double)memoryAvailable[i]/memoryTotal[i]*100 > 60){
			virDomainSetMemory(domains[i],memoryTotal[i]-memoryAvailable[i]/2);
			//memoryCollect += memoryAvailable[i]/2;		
		}
	}
	for(i=0;i<maxDomains;i++){
		//check if domain is starving,allocate memory to them
		if((double)memoryAvailable[i]/memoryTotal[i]*100 < 30){
			//let host allocates memory
			if((double)params[1].value/params[0].value*100 > 50){
				if(memoryTotal[i]+THRESHOLD300 > virDomainGetMaxMemory(domains[i]))
					virDomainSetMemory(domains[i],virDomainGetMaxMemory(domains[i]));				
				else
					virDomainSetMemory(domains[i],memoryTotal[i]+THRESHOLD300);
			}
			else{
				if(memoryTotal[i]+THRESHOLD200 > virDomainGetMaxMemory(domains[i]))
					virDomainSetMemory(domains[i],virDomainGetMaxMemory(domains[i]));				
				else
					virDomainSetMemory(domains[i],memoryTotal[i]+THRESHOLD200);			
			}
		}
	}
	//free memory
	free(params);
	free(memoryTotal);
	free(memoryAvailable);
	for(i = 0; i<maxDomains;i++){
		virDomainFree(domains[i]);	
	}
	free(domains);
	//sleep for timeInterval
	sleep(timeInterval);
  }
  //Close connection to Hypervisor
  if(virConnectClose(conn) < 0){
  	printf("Can't close connection to Hypervisor\n");
  	exit(1);
  }
  return 0;
}
  
