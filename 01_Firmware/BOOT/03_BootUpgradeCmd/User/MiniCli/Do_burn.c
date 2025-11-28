#include "command.h"

#include "ymodem.h"
#include "string.h"
#include "stdio.h"


uint8_t tab_1024[1024] = {0};

void Do_burn(int argc, char **argv)
{
    uint32_t ret_recv_counts = 0;
 
    if(argc < 2) return;

    if(strcmp(argv[1], "boot") == 0)
    {
 
    }else if(strcmp(argv[1], "app") == 0)
    {
		uint8_t Number[10] = "";
		int32_t Size = 0;
		Size = Ymodem_Receive(&tab_1024[0]);
		if (Size > 0)
		{
			printf("\r\n Update Over!\r\n");
			printf(" Name: ");
			printf("%s",file_name);
			Int2Str(Number, Size);
			printf("\r\n Size: ");
			printf("%s",Number);
			printf(" Bytes.\r\n");
			return;
		}
		else if (Size == -1)
		{
			printf("\r\n Image Too Big!\r\n");
			return;
		}
		else if (Size == -2)
		{
			printf("\r\n Update failed!\r\n");
			return;
		}
		else if (Size == -3)
		{
			printf("\r\n Aborted by user.\r\n");
			return;
		}
		else
		{
			printf(" Receive Filed.\r\n");
			return;
		}
    }
}