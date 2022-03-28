#include <stdio.h>
#include "scdef.h"

int main() {
  MFILE *mfp = mopen("data/iscript.bin");
  FILE *fp   = fopen("data/iscript.bin","rb");
  int i, end;
  byte buf[4096];

  if (!mfp || !fp)
    sc_err_fatal("failed on open\n");

  mseek(mfp, 0, SEEK_END);
  fseek(fp, 0, SEEK_END);

  if ((end = mtell(mfp)) != ftell(fp))
    sc_err_fatal("failed first seek and tell\n");

  printf("end: %d\n", end);

  /* write test */
  buf[0] = 0;
  buf[1] = 1;
  buf[2] = 2;
  buf[3] = 3;
  buf[4] = 4;
  buf[5] = 5;
  buf[6] = 6;
  buf[7] = 7;
  if (msave("test1.file", mfp) == -1)
    sc_err_fatal("save failed");
  if (mseek(mfp, end-8, SEEK_SET))
    sc_err_fatal("mseek failed");
  printf("tell: %d\n", mtell(mfp));
  if (mwrite(buf, 1, 8, mfp) != 8)
    sc_err_fatal("mwrite failed");
  printf("tell: %d\n", mtell(mfp));
  if (msave("test.file", mfp) == -1)
    sc_err_fatal("save failed");
  printf("tell: %d\n", mtell(mfp));
  mresize(mfp, end-8);
  printf("tell: %d\n", mtell(mfp));
  if (msave("test.resized.file", mfp) == -1)
    sc_err_fatal("save 2 failed");
  printf("tell: %d\n", mtell(mfp));
  
  /* disable this test for now */
  for (i=end; i<end; i++) {
    int j;
    if (mseek(mfp, i, SEEK_SET) != 0 || fseek(fp, i, SEEK_SET))
      sc_err_fatal("failed on seek number %d\n", i);
    for (j=1023; j<1024; j++) {
      int o, p;
      fprintf(stderr,"current position: %d\n", (int)mtell(mfp));
      o=mread(buf, 1, j, mfp);
      p=fread(buf, 1, j, fp);

      if (o!=p) {
	fprintf(stderr,"read %d bytes: supposed to be %d\n", o, p);
	sc_err_fatal("failed on byte read of nobj %d number %d\n", j, i);
      }
      if (mseek(mfp, -1*j, SEEK_CUR) != 0 || fseek(fp, -1*j, SEEK_CUR))
	sc_err_fatal("failed on rev byte seek number i=%d j=%d\n", i, j);
      o=mread(buf, 2, j, mfp);
      p=fread(buf, 2, j, fp);
      
      if (o!=p) {
	fprintf(stderr,"read %d uint16s: supposed to be %d\n", o, p);
	sc_err_fatal("failed on uint16 read of nobj %d number %d\n", j, i);
      }
      if (mseek(mfp, -2*j, SEEK_CUR) != 0 || fseek(fp, -2*j, SEEK_CUR))
	sc_err_fatal("failed on rev uint16 seek number i=%d j=%d\n", i, j);
      o=mread(buf, 4, j, mfp);
      p=fread(buf, 4, j, fp);
      
      
      if (o!=p) {
	fprintf(stderr,"read %d uint32s: supposed to be %d\n", o, p);
	sc_err_fatal("failed on uint32 read of nobj %d number %d\n", j, i);
      }
      if (mseek(mfp, -4*j, SEEK_CUR) != 0 || fseek(fp, -4*j, SEEK_CUR))
	sc_err_fatal("failed on rev uint32 seek number i=%d j=%d\n", i, j);
    }
    if (mtell(mfp) != ftell(fp)) {
      mseek(mfp, ftell(fp), SEEK_SET);
      /*fprintf(stderr,"pos is %d but should be %d\n",(int)mtell(mfp),(int)ftell(fp));
	sc_err_fatal("failed on tell number %d\n", i);*/
    }
  }

  if (mclose(mfp) != fclose(fp))
    sc_err_fatal("failed on close\n");

  return 0;
}
