#include <stdio.h>
#include <string.h>

#define BUFSIZE 300
#define READ	0
#define WRITE	1
#define SLOT_SIZE 5120	//GB 5TB

void msr2ascii(char *source,char *target,char *output);

int main()
{
	msr2ascii("msr/src2_2.csv","ascii/src2_2.ascii","output/src2_2.txt");
	msr2ascii("msr/usr_2.csv","ascii/usr_2.ascii","output/usr_2.txt");
	msr2ascii("msr/src1_1.csv","ascii/src1_1.ascii","output/src1_1.txt");
	msr2ascii("msr/proj_1.csv","ascii/proj_1.ascii","output/proj_1.txt");
	msr2ascii("msr/prn_1.csv","ascii/prn_1.ascii","output/prn_1.txt");
	msr2ascii("msr/proj_2.csv","ascii/proj_2.ascii","output/proj_2.txt");
	msr2ascii("msr/usr_1.csv","ascii/usr_1.ascii","output/usr_1.txt");
	msr2ascii("msr/src1_0.csv","ascii/src1_0.ascii","output/src1_0.txt");
	msr2ascii("msr/prxy_1.csv","ascii/prxy_1.ascii","output/prxy_1.txt");
	return 1;
}

void msr2ascii(char *source,char *target,char *output)
{
	char buf[BUFSIZE];
	FILE *file_source=fopen(source,"r");
	FILE *file_target=fopen(target,"w");
	FILE *file_output=fopen(output,"w");

	//ASCII Format
//	double time;		//milliseconds
//	unsigned int dev;
	long long lba;		//LBA
	unsigned int size;	//Blocks
	unsigned int type;	//R<->0;W<->1

	//MSR Format
	long long timeStamp;	//windows filetime (100ns)
	char hostName[10];		
	unsigned int diskNumber;
	char operation[10];		//"Read" or "Write"
	long long offset;	//bytes
	unsigned int length;	//bytes
	long long responseTime;	//windows filetime

	//for preprocess
	int i;
	unsigned int count_all=0;
	unsigned int count_wrt=0;
	long long size_all=0;
	long long size_wrt=0;
	unsigned int zone_call=0;
	unsigned int zone_cwrt=0;
	unsigned int zone_id;
	unsigned int zone_all[30000];
	unsigned int zone_wrt[30000];
	memset(zone_all,0,sizeof(unsigned int)*30000);
	memset(zone_wrt,0,sizeof(unsigned int)*30000);
	
	printf("Format Coverting %s ...\n",source);
	while(fgets(buf,sizeof(buf),file_source))
	{
		for(i=0;i<sizeof(buf);i++)
		{
			if(buf[i]==',')
			{
				buf[i]=' ';
			}
		}
		sscanf(buf,"%lld %s %d %s %lld %d %lld",&timeStamp,hostName,
			&diskNumber,operation,&offset,&length,&responseTime);
		
		//Format Covert
		lba=offset/512;
		size=length/512;
		
		lba = lba/8;
		size = (size-1)/8 + 1;
//		size = size*5;
		
		count_all++;
		size_all+=size;
		
		zone_id = lba/65536;
		if(zone_all[zone_id] == 0)
		{
			zone_call++;
			zone_all[zone_id] =1;
		}
		
		if(strcmp(operation,"Read")==0)
		{
			type=READ;
		}else
		{
			type=WRITE;
			count_wrt++;
			size_wrt+=size;
			if(zone_wrt[zone_id] == 0)
			{
				zone_cwrt++;
				zone_wrt[zone_id] =1;
			}
		}
		
		/*************************/
		if(lba > (long long)SLOT_SIZE*1024*256)//5TB
		{
			lba=lba%((long long)SLOT_SIZE*1024*256);
			printf("--##--A Req. is Out of Range 5TB--##--\n");
		}
		if(size > 256)//1MB
		{
			size=256;
			printf("--++--A Size is Out of Range 1MB--++--\n");
		}
		/*************************/
		fprintf(file_target,"%-8lld %-4d %d\n",lba,size,type);
		fflush(file_target);
	}//while
	fclose(file_source);
	fclose(file_target);
	
	printf("------------------------------------\n");
	printf("total I/O Request = %d\n",count_all);
	printf("total Wrt Request = %d\n",count_wrt);
	printf("total I/O Blocks = %lld	(%Lf GB)\n",size_all,(long double)size_all/256/1024);
	printf("total Wrt Blocks = %lld	(%Lf GB)\n",size_wrt,(long double)size_wrt/256/1024);
	printf("total I/O Zones = %d\n",zone_call);
	printf("total Wrt Zones = %d\n",zone_cwrt);
	printf("------------------------------------\n");
	
	fprintf(file_output,"total I/O Request = %d\n",count_all);
	fprintf(file_output,"total Wrt Request = %d\n",count_wrt);
	fprintf(file_output,"total I/O Blocks = %lld	(%Lf GB)\n",size_all,(long double)size_all/256/1024);
	fprintf(file_output,"total Wrt Blocks = %lld	(%Lf GB)\n",size_wrt,(long double)size_wrt/256/1024);
	fprintf(file_output,"total I/O Zones = %d\n",zone_call);
	fprintf(file_output,"total Wrt Zones = %d\n",zone_cwrt);
	
}
