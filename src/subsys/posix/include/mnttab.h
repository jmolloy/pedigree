#ifndef MNTTAB_H
#define MNTTAB_H

#define MNTTAB "/etc/mnttab"
#define MNT_LINE_MAX 1024

#define MNT_TOOLONG 1/* entry exceeds MNT_LINE_MAX */
#define MNT_TOOMANY 2/* too many fields in line */
#define MNT_TOOFEW 3/* too few fields in line */ 


struct mnttab {
  char mt_dev[32], mt_filsys[32];
  short mt_ro_flg;
  long mt_time;
};
#endif
