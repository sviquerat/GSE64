#define AUDIO_VOR		1

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <unistd.h>
#include <getopt.h>

#define DBG 0

//some constants and functions
#define R_EARTH 6.3781E6 // radius of earth in meters
#define TO_RAD (M_PI / 180) // conversion from degrees to rad

#if AUDIO_VOR

/* Audio VOR format */
#define SIG_EFF_MAX_FILES		26
#define SIG_EFF_SUFFIX(n)		('a' + (n))
#define SIG_EFF_FORMAT			"%c"

#else

/* Original VOR format */
#define SIG_EFF_MAX_FILES		100
#define SIG_EFF_SUFFIX(n)		(n)
#define SIG_EFF_FORMAT			"%02d"

#endif

/* GLOBAL VARS */
char GPSfile[80], SIGfile[80], EFFfile[80], GSEfile[80];// vars storing the 4 filenames produced by VOR
FILE *fpGPS, *fpSIG, *fpEFF, *fpGSE;			// file pointer to files
char *filename;
char *distance_calculation;				// Method: HAVER - Haversine (default) OLD: Old version
char timestring[10];					// timestring that will be filled with data
char GPSdata[7][80];					// Datenfelder in der GPSfile
char GPStime[10];					// timestring in GPSfile
char SIGdata[21][80];					// Datenfelder in der SIGfile
char SIGtime[10];					// timestring in SIGfile
int  SIGfilecntr = 0;					// counter for SIG files
char EFFdata[14][80];					// Datenfelder in der EFF-Datei
char EFFcopy[14][80];					// Kopie der letzt geschriebenen EFF-Datenfelder
char EFFtime[10];					// timestring in EFFfile
int  EFFfilecntr = 0;					// counter for EFFfiles
int  rc;						// return code
int  id = 0;						// index of output file
int  First_EFF_B_Found = 0;				// Merker fuer erstes B in EFF-Datei
int  PauseGSEOutput = 0;				// Merker fuer Pausieren der Ausgabe E -> B
int  interval=4;					// merge interval

void strip_ext(char *fname)
{
    char *end = fname + strlen(fname);

    while (end > fname && *end != '.' && *end != '\\' && *end != '/') {
        --end;
    }
    if ((end > fname && *end == '.') &&
        (*(end - 1) != '\\' && *(end - 1) != '/')) {
        *end = '\0';
    }  
}

/* Hilfetext schreiben */
int showhelp(void)
{
    printf("Das Program liesst die GPS-, SIG- und EFF-Dateien ein um daraus eine neue Datei\n"
           "mit der Endung GSE zu schreiben, die die Eintraege synchronisiert.\n"
           "Aufruf mit z.B.: GSEMERGE 030807\n");
           return(1);
}

double dist_haversine(double lon1, double lat1, double lon2, double lat2)
{
	double dx, dy, dz;
	double ph1 = lat1;
	double ph2 = lat2;
	double th1 = lon1;
	double th2 = lon2;
	
	ph1 -= ph2;
	ph1 *= TO_RAD, th1 *= TO_RAD, th2 *= TO_RAD;
 
	dz = sin(th1) - sin(th2);
	dx = cos(ph1) * cos(th1) - cos(th2);
	dy = sin(ph1) * cos(th1);
	return asin(sqrt(dx * dx + dy * dy + dz * dz) / 2) * 2 * R_EARTH/1000; //output is in km
}

double dist_old(double lon1, double lat1, double lon2, double lat2)
{
	double phi1 = lat1*TO_RAD;
	double phi2 = lat2*TO_RAD;
	double lambda1 = lon1*TO_RAD;
	double lambda2 = lon2*TO_RAD;
	double dphi = phi2-phi1;
	double dlambda = lambda2-lambda1;
	double a = pow(sin(dphi/2),2)+cos(phi1)*cos(phi2)*pow(sin(dlambda/2),2); //pow(x,2) is x^2
	double c = 2*atan2(sqrt(a),sqrt(1-a));
	return R_EARTH/1000 * c; //get km
}

int writeGPS(void)
{
	static double lat1, lon1;
	double lat2, lon2, dist;
	if((id == 0) || (EFFdata[2][0] == 'B') || (EFFdata[2][0] == 'R')){
		lat1 = strtod(GPSdata[1], NULL);
		lon1 = strtod(GPSdata[2], NULL);
		dist = 0;
	}
	else{
		lat2 = strtod(GPSdata[1], NULL);
		lon2 = strtod(GPSdata[2], NULL);
	
		if (strcmp(distance_calculation, "HAVER") == 0)
		{
			dist = dist_haversine(lon1,lat1,lon2,lat2);
		}
		else
		{
			dist = dist_old(lon1,lat1,lon2,lat2);
		}
	}
	if(!First_EFF_B_Found) return(1);   /* GSE-Datei erst mit erstem B in EFF-Datei schreiben */
	if(PauseGSEOutput)     return(1);   /* GSE-Datei-Ausgabe anhalten E -> B */
	fprintf(fpGSE, "%04d", ++id);
	fprintf(fpGSE, "\t%s\t%s\t%s\t%6.3f\t%s", GPSdata[0], GPSdata[1], GPSdata[2], dist, GPSdata[3]);
    return(1);
}

int writeSIG(void)
{
    if(!First_EFF_B_Found) return(1);   /* GSE-Datei erst mit erstem B in EFF-Datei schreiben */
    if(PauseGSEOutput)     return(1);   /* GSE-Datei-Ausgabe anhalten E -> B */
    fprintf(fpGSE, "\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s",
        SIGdata[0], SIGdata[1], SIGdata[2], SIGdata[3], SIGdata[4], SIGdata[5], SIGdata[6],
        SIGdata[7], SIGdata[8], SIGdata[9], SIGdata[10], SIGdata[11], SIGdata[12], SIGdata[13],
        SIGdata[14], SIGdata[15], SIGdata[17], SIGdata[19]);
    return(1);
}

int writeSIGdummies(void)
{
    if(!First_EFF_B_Found) return(1);   /* GSE-Datei erst mit erstem B in EFF-Datei schreiben */
    if(PauseGSEOutput)     return(1);   /* GSE-Datei-Ausgabe anhalten E -> B */
    fprintf(fpGSE, "\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t");
    return(1);
}

int writeEFF(void)
{
    if(!First_EFF_B_Found) return(1);   /* GSE-Datei erst mit erstem B in EFF-Datei schreiben */
    if(PauseGSEOutput)     return(1);   /* GSE-Datei-Ausgabe anhalten E -> B */
    fprintf(fpGSE, "\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s",
        EFFdata[0],EFFdata[1],EFFdata[2], EFFdata[4], EFFdata[5], EFFdata[6],   /* 31.01.04 UK [0] und [1] zugefuegt */
        EFFdata[7], EFFdata[8], EFFdata[9], EFFdata[10], EFFdata[11], EFFdata[12]);
    fprintf(fpGSE, "\n");
    memcpy(EFFcopy, EFFdata, sizeof(EFFcopy));
    return(1);
}

int writeEFFcopy(void)
{
    if(!First_EFF_B_Found) return(1);   /* GSE-Datei erst mit erstem B in EFF-Datei schreiben */
    if(PauseGSEOutput)     return(1);   /* GSE-Datei-Ausgabe anhalten E -> B */
    fprintf(fpGSE, "\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s",
        EFFcopy[0],EFFcopy[1],EFFcopy[2], EFFcopy[4], EFFcopy[5], EFFcopy[6],       /* 31.01.04 UK [0] und [1] zugefuegt */
        EFFcopy[7], EFFcopy[8], EFFcopy[9], EFFcopy[10], EFFcopy[11], EFFcopy[12]); /* 21 Feb 2004 JD Korrektur */
    fprintf(fpGSE, "\n");
    return(1);
}

int writeEFFdummies(void)
{
    if(!First_EFF_B_Found) return(1);   /* GSE-Datei erst mit erstem B in EFF-Datei schreiben */
    if(PauseGSEOutput)     return(1);   /* GSE-Datei-Ausgabe anhalten E -> B */
    fprintf(fpGSE, "\t\t\t\t\t\t\t\t\t\t\t\t\n"); /* 31.01.04 UK \t\t zugefuegt */
    return(1);
}

int writeGPStime(int hh, int mm, int ss)
{
int s;
int n;

    for (n = 0; n < 4; n++) {
        s = ss + n;
        sprintf(timestring, "%02d:%02d:%02d", hh, mm, s);
        if(strstr(GPSdata[0], timestring)){
            //printf("%d %s\n", n, timestring);
            return(1); /* Gefunden */
        }
    } /* endfor */
    return (0); /* Nicht gefunden */
}

int readGPSline(void)
{
char line[200];
char *p;
const char *delims = {","};
int  n = 0;

    if (NULL == fgets (line, 200, fpGPS)) return (0); /* Bis Newline lesen */
    //printf("%s\n", line);

    p = strtok(line, delims);
    while(*p == ' ') p++;  /* Fhrende Blanks l”schen */
    sprintf(GPSdata[n], "%s", p);
    while((p != NULL) && (n < 6)){
        n++;
        p = strtok(NULL, delims);
        while(*p == ' ') p++;  /* Fhrende Blanks l”schen */
        sprintf(GPSdata[n], "%s", p);
    }
    sprintf(GPStime, "%s", (GPSdata[0]+11));

    return(1);
}

int findSIGtime(int hh, int mm, int ss)
{
int s;
int n;

    for (n = 0; n < 4; n++) {
        s = ss + n;
        sprintf(timestring, "%02d:%02d:%02d", hh, mm, s);
        if(strstr(SIGtime, timestring)){
            return(1);
        }
    }
    return (0);
}

int readSIGline(void)
{
char line[200];
char *p;
int  n, m;

    if (NULL == fgets (line, 200, fpSIG)){
        fclose(fpSIG);
#if !AUDIO_VOR
        filename[6] = 0;
#endif

        do {
            SIGfilecntr++;
            sprintf(SIGfile, "%s" SIG_EFF_FORMAT ".SIG", filename, SIG_EFF_SUFFIX(SIGfilecntr));
            fpSIG = fopen(SIGfile, "r");
        } while((fpSIG == NULL) && (SIGfilecntr < SIG_EFF_MAX_FILES - 1));

        if (fpSIG == NULL) {
            return(0);
        }
        printf("reading file %s...\n", SIGfile);
        if (NULL == fgets (line, 200, fpSIG)) return (0);
    }
#if DBG
    printf("SIG: %s\n", line);
#endif
    p = line;
    n = 0;
    while ((*p != 0) && (*p != 0x0d) && (*p != 0x0a)) {
        if (*p == '"') n++;
        if ((*p == ',') && ((n % 2) == 1)) *p = ';';
        p++;
    }
    p = line;
    n = 0;
    while (n < 20) {
        m = 0;
        SIGdata[n][m] = 0;
        while ((*p != ',') && (*p != 0) && (*p != 0x0d) && (*p != 0x0a)) {
            if (*p != '"') {
                SIGdata[n][m] = *p;
                m++;
            }
            p++;
        }
        SIGdata[n][m] = 0;
        if(*p == ',') {
            p++;
        }
        n++;
    }
    sprintf(SIGtime, "%s", (SIGdata[3]+11));

    return(1);
}

int findeFFtime(int hh, int mm, int ss)
{
int s;
int n;

    for (n = 0; n < 4; n++) {
        s = ss + n;
        sprintf(timestring, "%02d:%02d:%02d", hh, mm, s);
        if(strstr(EFFtime, timestring)){
            if (EFFdata[2][0] == 'E') PauseGSEOutput = 1;   /* Anhalten     */
            if (EFFdata[2][0] == 'C') PauseGSEOutput = 1;   /* Anhalten     */
            if (EFFdata[2][0] == 'B') PauseGSEOutput = 0;   /* Weitermachen */
            if (EFFdata[2][0] == 'R') PauseGSEOutput = 0;   /* Weitermachen */
            return(1);
        }
    }
    return (0);
}

int readEFFtime(void)
{
char line[200];
char *p;
int  n, m;

    if (NULL == fgets (line, 200, fpEFF)){
#if DBG
		printf("At end of eff file\n");
#endif
        fclose(fpEFF);
#if !AUDIO_VOR
			filename[6] = 0;
#endif

        do {
            EFFfilecntr++;
            sprintf(EFFfile, "%s" SIG_EFF_FORMAT ".EFF", filename, SIG_EFF_SUFFIX(EFFfilecntr));
#if DBG
		printf("Trying file %s\n",EFFfile);
#endif
				fpEFF = fopen(EFFfile, "r");
        } while((fpEFF == NULL) && (EFFfilecntr < SIG_EFF_MAX_FILES - 1));

        if (fpEFF == NULL) {
            return(0);
        }
        printf("reading file %s...\n", EFFfile);
        if (NULL == fgets (line, 200, fpEFF)) return (0);
    }
#if DBG
    printf("EFF line: %s\n", line);
#endif
    p = line;
    n = 0;
    while (n < 13) {
        m = 0;
        EFFdata[n][m] = 0;
        while ((*p != ',') && (*p != 0) && (*p != 0x0d) && (*p != 0x0a)) {
            if (*p != '"') {
                EFFdata[n][m] = *p;
                m++;
            }
            p++;
            if ((n == 12) && (*p == ',')) {
                EFFdata[n][m] = *p;
                m++;
                p++;
            }
        }
        EFFdata[n][m] = 0;
        if(*p == ',') {
            p++;
        }
#if AUDIO_VOR
        if ((n == 10)) {
            n++;
            if (m > 1)
            	{
            	EFFdata[n][0] = EFFdata[n-1][1];
            	EFFdata[n][1] = 0;
					EFFdata[n-1][1] = 0;
					}
            else
            	EFFdata[n][0] = 0;
        }
#else
        if ((n == 10) && (m == 0)) {
            n++;
            EFFdata[n][m] = 0;
        }

#endif
        n++;
        if (((EFFdata[2][0] == 'C') || (EFFdata[2][0] == 'R') || (EFFdata[2][0] == 'E')) && (n > 3)) {
            n = 13;
        }
    }
    sprintf(EFFtime, "%s", (EFFdata[3]+11));

#if DBG
for (n = 0; n < 13; ++n)
	{
	printf("Field %d: %s\n",n,EFFdata[n]);
	}
#endif

    if (EFFdata[2][0] == 'A') {
        for (n = 4; n < 13; n++) {
            if (EFFdata[n][0] == 0) {
                sprintf(EFFdata[n], "%s", EFFcopy[n]);
            }
        }
    }
    return(1);
}

int gsemain(void)
{
int hh, mm, ss;
int SIGfound;
int wiederholeSIGintervall = 0;

    readGPSline();
    readSIGline();

    do {
        readEFFtime();
    } while (EFFdata[2][0] != 'B'); /* enddo */

    for (hh = 0; hh < 24; hh++) {
        for (mm = 0; mm < 60; mm++) {
            for (ss = 0; ss < 60; ss += interval) {
                if (writeGPStime(hh, mm, ss)) {
                    sprintf(timestring, "%02d:%02d:%02d", hh, mm, ss);
                    do {
                        if (findeFFtime(hh, mm, ss)) First_EFF_B_Found = 1;
                        writeGPS();

                        if (findSIGtime(hh, mm, ss)) {
                            SIGfound = 1;
                            writeSIG();
                            if (readSIGline()) {
                                if (findSIGtime(hh, mm, ss)){
                                    wiederholeSIGintervall = 1;
                                } else {
                                    wiederholeSIGintervall = 0;
                                }
                            }

                        } else {
                            SIGfound = 0;
                            writeSIGdummies();
                            while (strcmp(GPStime, SIGtime) > 0) {
                                if (!readSIGline()) break; //return (1);
                            }
                        }

                        if (findeFFtime(hh, mm, ss) && (EFFdata[2][0] != 'C')) {
                            writeEFF();
                            if (!readEFFtime()) return (1);
                            while (findeFFtime(hh, mm, ss)){
                                if (!readEFFtime()) return (1);
                            }
                        } else {
                            if (SIGfound) {
                                writeEFFcopy();
                            } else {
                                writeEFFcopy();
                            }
                            while (strcmp(GPStime, EFFtime) > 0) {
                                if (!readEFFtime()) return (1);
                            }
                        }

                    } while (wiederholeSIGintervall == 1);
					
                    if (!readGPSline()) return (1);
                    
					while (writeGPStime(hh, mm, ss)){
                        if (!readGPSline()) return (1);
                    }

                }
            }
        }
    }
    return(1);
}

int main(int argc, char ** argv)
{
	static const struct option long_options[] =
    {
        { "gsefile", required_argument,0, 'f' },
        { "interval", optional_argument,0, 'i' },
        { "distance", optional_argument,0, 'd' },
		{0}
    };
	
	int c,index,val;
	distance_calculation = "HAVER";
	
	while ((c = getopt_long(argc, argv,"f:i::d::",long_options, &index)) != -1)
    switch (c)
      {
		case 'f':
			filename = optarg;
			break;
		case 'i': 
			val = atoi(optarg); //this should be left and right trimmed
			if (val <1)
			{
				printf("Take care on how interval is provided!\nDefaulting to 4 seconds.\n");
				interval = 4;
			}
			else
			{
				interval=val;
			}
			break;
		case 'd': 
			distance_calculation = optarg;
			break;
		case 0: 
			showhelp();
			return(1);
		default: 
			abort();
	  }
	
       
    printf("%s Reworked by Sacha Viquerat, 2024. Original work (c) Geo-X\n", argv[0]);
	printf("Setting coordinate interval to %d seconds\n", interval);
	
    strip_ext(filename); //in place removal of file extension
    sprintf(GPSfile, "%s.GPS",   filename);
    sprintf(GSEfile, "%s.GSE",   filename);

    fpGPS = fopen(GPSfile, "r");
	
    if (fpGPS == NULL) {
        printf("Konnte Datei %s nicht finden!\n", GPSfile);
        return(1);
    } /* endif */

    do {
        sprintf(SIGfile, "%s" SIG_EFF_FORMAT ".SIG", filename, SIG_EFF_SUFFIX(SIGfilecntr));  /* Name aus filename und lfd. Nummer bauen */
        fpSIG = fopen(SIGfile, "r");                            /* Versuche Datei zu oeffnen               */
        if (fpSIG == NULL) SIGfilecntr++;                       /* Wenn gescheitert die Nummer erhoehen    */
    } while((fpSIG == NULL) && (SIGfilecntr < SIG_EFF_MAX_FILES - 1));            /* Wiederhole Versuch max. 99-mal          */
    if (fpSIG == NULL) {
        printf("Konnte Datei %s nicht finden!\n", SIGfile);
        fclose(fpGPS);
        return(1);
    } /* endif */

    do {
        sprintf(EFFfile, "%s" SIG_EFF_FORMAT ".EFF", filename, SIG_EFF_SUFFIX(EFFfilecntr));  /* Name aus filename und lfd. Nummer bauen */
        fpEFF = fopen(EFFfile, "r");                            /* Versuche Datei zu oeffnen               */
        if (fpEFF == NULL) EFFfilecntr++;                       /* Wenn gescheitert die Nummer erhoehen    */
    } while((fpEFF == NULL) && (EFFfilecntr < SIG_EFF_MAX_FILES - 1));            /* Wiederhole Versuch max. 99-mal          */
    
	if (fpEFF == NULL) {
        printf("Konnte Datei %s nicht finden!\n", EFFfile);
        fclose(fpGPS);
        fclose(fpSIG);
        return(1);
    } /* endif */

    fpGSE = fopen(GSEfile, "wt");
    if (fpGSE == NULL) {
        printf("Konnte Datei %s nicht erzeugen!\n", GSEfile);
        fclose(fpGPS);
        fclose(fpSIG);
        fclose(fpEFF);
        return(1);
    } /* endif */

    fprintf(fpGSE, "Nr\tGPS_Zeit\tLat\tLon\tDistance\tHeight"
                   "\tTrack\tLeg\tFlight_nr\tSIG_time\tObserver\tAngle"
                   "\tSpecies\tSIG_Number\tCue\tSwim_dir\tReaction\tDive\tCom_Sight"
                   "\tSide\tPos_N\tPos_E\tBehaviour\tCalves"
                   "\tEFF_Track\tEFF_Leg"
                   "\tEffort\tSeastate\tTurbidity\tCloudCover\tAngle_1\tAngle_2\tGlare\tsub_l\tsub_r\tCommentEFF\n");

    printf("Lese Datei %s, %s, %s...\n", GPSfile, SIGfile, EFFfile);
    rc = gsemain();
    fclose(fpGPS);
    fclose(fpSIG);
    fclose(fpEFF);
    fclose(fpGSE);
    printf("Datei %s konnte erfolgreich erstellt werden.\n", GSEfile);
    return(rc);
}
