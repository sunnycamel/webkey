#include "suinput.h"
#include <stdio.h>
int main(int argc, char **argv)
{

	void *pkey;
	pkey = EVP_PKEY_new();
	FILE f = fopen(pem,"w");
	if (!f)
		return 0;
	PEM_write_PrivateKey(f,pkey,EVP_des_ede3_cbc(),NULL,0,NULL,NULL);
	fclose(f);

  return 0;
}
