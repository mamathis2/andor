//------------------------------------------------------------------------------
//  PROJECT:		32-bit Driver Example Code ---- Photon Counting
//                                              
//  Copyright 2010. All Rights Reserved
//
//  FILE:				photoncounting.c
//  AUTHOR:			Brendan O'Neill
//
//  OVERVIEW:		This Project shows how to set up the Andor MCD to take a single
//							image acquisition and display it on the screen. It also explores
//							the cooling functions available and how to safely handle them.
//              The most important thing to remember is to raise the temp to
//							above 0 to stop damage to the head when exiting the program.
//							It will	familiarise you with using the Andor MCD driver library.
//------------------------------------------------------------------------------

#include <windows.h>            // required for all Windows applications
#include <stdio.h>              // required for sprintf()
#include <math.h>               // required for ceil()
#include "atmcd32d.h"           // Andor function definitions

#ifndef WINVER
#define WINVER 0x0500
#endif
#define Color 256                      // Number of colors in the palette

// Function Prototypes
BOOL CreateWindows(void);         // Create control windows and allocate handles
void SetupWindows(void);          // Initialize control windows
void SetWindowsToDefault(char[256]);// Fills windows with default values
void SetSystem(void);             // Sets hardware parameters
void ProcessTimer(WPARAM);        // Handles WM_TIMER messages
void ProcessPushButtons(LPARAM);  // Processes button presses
void SwitchCoolerOn(void);        // Handles temp and cooler functions
void SwitchCoolerOff(void);       // Switches cooler off
void UpdateDialogWindows(void);   // refreshes all windows
void FillRectangle(void);         // clears paint area
BOOL AcquireImageData(void);      // Acquires data from card
void PaintDataWindow(void);       // Prepares paint area on screen
BOOL DrawLines(long*,long*);      // paints data to screen
int AllocateBuffers(void);        // Allocates memory for buffers
void FreeBuffers(void);           // Frees allocated memory
void PaintImage(long maxValue, long minValue, int Start); //Display data on screen
void CreateIdentityPalette(HDC ScreenDC); //Palette for PaintData()
BOOL ProcessMessages(UINT message, WPARAM wparam, LPARAM lparam){return FALSE;} // No messages to process in this example

// Set up acquisition parameters here to be set in common.c *****************
int acquisitionMode=1;
int readMode=4;
int xWidth=630;    // width of application window passed to common.c
int yHeight=500;   // height of application window passed to common.c
//******************************************************************************

extern AndorCapabilities caps;         // Get AndorCapabilities structure from common.c
extern char              model[32];    // Get Head Model from common.c
extern int 	             gblXPixels;   // Get dims from common.c
extern int               gblYPixels;
extern int               VSnumber;     // Get speeds from common.c
extern int               HSnumber;
extern int               ADnumber;

// Declare Image Buffers
long *pImageArray = NULL;// main image buffer read from card
long *pOutputImage = NULL;

int gblStatusTimer=100;  // ID of timer that checks status before acquisition
int gblTempTimer=200;    // ID of timer that updates temperature status

BOOL errorFlag;				   // Tells us if initialization failed in common.c
BOOL gblData=FALSE;    	 // flag is set when first acquisition is taken, tells
											 	 //	system that there is data to display

BOOL gblCooler=FALSE;    // Indicate if cooler is on
RECT rect;             	 // Dims of paint area

extern HWND		hwnd; 		 // Handle to main application

HWND          ebInit,    // handles for the individual
              stInit,    // windows such as edit boxes
              ebExposure,// and comboboxes etc.
              stExposure,
              ebTemp,
              stTemp,
              stPhotonThreshold,
              ebPhotonThreshold1,
              stPhotonThreshold1,
              ebPhotonThreshold2,
              stPhotonThreshold2,
              ebPhotonThreshold3,
              stPhotonThreshold3,
              pbCoolerOn,
              pbCoolerOff,
              stActTemp,
              ebStatus,
              stStatus,
              pbStart,
              pbAbort,
              pbClose,
              pbPostProcess;

extern HINSTANCE hInst;    // Current Instance

//------------------------------------------------------------------------------
//	FUNCTION NAME:	CreateWindows()
//
//  RETURNS:				TRUE: Successful
//									FALSE: Unsuccessful
//
//  LAST MODIFIED:	PMcK	09/11/98
//
//  DESCRIPTION:    This function creates the individual controls placed in the
//									main window. i.e. Comboboxes, edit boxes etc. When they are
//									created they are issued a handle which is stored in it's
//									global variable.
//
//	ARGUMENTS: 			NONE
//------------------------------------------------------------------------------

BOOL CreateWindows(void)
{
  HINSTANCE 	hInstance=hInst;

  // Create windows for each control and store the handle names
  stInit=CreateWindow("STATIC","Initialization Information",
                        WS_CHILD|WS_VISIBLE|SS_LEFT,
                        10,2,180,18,hwnd,0,hInstance,NULL);
  ebInit=CreateWindow("EDIT","",
                        WS_CHILD|WS_VISIBLE|WS_BORDER|ES_LEFT,
                        10,20,320,40,hwnd,0,hInstance,NULL);
  stExposure=CreateWindow("STATIC","Exposure time (secs):",
                        WS_CHILD|WS_VISIBLE|SS_LEFT,
                        10,80,160,20,hwnd,0,hInstance,NULL);
  ebExposure=CreateWindow("EDIT","",
                        WS_CHILD|WS_VISIBLE|WS_BORDER|ES_LEFT,
                        250,80,50,20,hwnd,0,hInstance,NULL);
  stPhotonThreshold=CreateWindow("STATIC","Photon Counting Thresholds:",
                        WS_CHILD|WS_VISIBLE|SS_LEFT,
                        10,110,250,20,hwnd,0,hInstance,NULL);
  stPhotonThreshold1=CreateWindow("STATIC","Th1:",
                        WS_CHILD|WS_VISIBLE|SS_LEFT,
                        10,140,50,20,hwnd,0,hInstance,NULL);
  ebPhotonThreshold1=CreateWindow("EDIT","",
                        WS_CHILD|WS_VISIBLE|WS_BORDER|ES_LEFT,
                        60,140,50,20,hwnd,0,hInstance,NULL);
  stPhotonThreshold2=CreateWindow("STATIC","Th2:",
                        WS_CHILD|WS_VISIBLE|SS_LEFT,
                        120,140,50,20,hwnd,0,hInstance,NULL);
  ebPhotonThreshold2=CreateWindow("EDIT","",
                        WS_CHILD|WS_VISIBLE|WS_BORDER|ES_LEFT,
                        170,140,50,20,hwnd,0,hInstance,NULL);
  stPhotonThreshold3=CreateWindow("STATIC","Th3:",
                        WS_CHILD|WS_VISIBLE|SS_LEFT,
                        230,140,50,20,hwnd,0,hInstance,NULL);
  ebPhotonThreshold3=CreateWindow("EDIT","",
                        WS_CHILD|WS_VISIBLE|WS_BORDER|ES_LEFT,
                        280,140,50,20,hwnd,0,hInstance,NULL);
  stTemp=CreateWindow("STATIC","Temperature Setting (C):",
                        WS_CHILD|WS_VISIBLE|SS_LEFT,
                        10,170,180,20,hwnd,0,hInstance,NULL);
  ebTemp=CreateWindow("EDIT","",
                        WS_CHILD|WS_VISIBLE|WS_BORDER|ES_LEFT,
                        250,170,50,20,hwnd,0,hInstance,NULL);
  pbCoolerOn=CreateWindow("BUTTON","Set temp",
                        WS_CHILD|WS_VISIBLE|WS_BORDER|BS_PUSHBUTTON,
                        10,230,90,25,hwnd,0,hInstance,NULL);
  pbCoolerOff=CreateWindow("BUTTON","Cooler OFF",
                        WS_CHILD|WS_VISIBLE|WS_BORDER|BS_PUSHBUTTON,
                        120,230,90,25,hwnd,0,hInstance,NULL);
  stActTemp=CreateWindow("STATIC","Temperature control is disabled",
                        WS_CHILD|WS_VISIBLE|SS_LEFT,
                        10,210,300,20,hwnd,0,hInstance,NULL);
  ebStatus=CreateWindow("EDIT","",
                        WS_CHILD|WS_VISIBLE|WS_BORDER|ES_LEFT|ES_MULTILINE,
												10,310,600,150,hwnd,0,hInstance,NULL);
  pbStart=CreateWindow("BUTTON","Start Acq",
                        WS_CHILD|WS_VISIBLE|WS_BORDER|BS_PUSHBUTTON,
                        10,260,90,25,hwnd,0,hInstance,NULL);
  pbAbort=CreateWindow("BUTTON","Abort Acq",
                        WS_CHILD|WS_VISIBLE|WS_BORDER|BS_PUSHBUTTON,
                        120,260,90,25,hwnd,0,hInstance,NULL);
  pbClose=CreateWindow("BUTTON","Close",
                        WS_CHILD|WS_VISIBLE|WS_BORDER|BS_PUSHBUTTON,
                        240,260,90,25,hwnd,0,hInstance,NULL);
  pbPostProcess=CreateWindow("BUTTON","Post Process",
                        WS_CHILD|WS_VISIBLE|WS_BORDER|BS_PUSHBUTTON,
                        240,230,95,25,hwnd,0,hInstance,NULL);
  stStatus=CreateWindow("STATIC","Status",
                        WS_CHILD|WS_VISIBLE|SS_LEFT,
												10,290,60,18,hwnd,0,hInstance,NULL);

  SetupWindows();      // fill windows with default data

  return TRUE;

}

//------------------------------------------------------------------------------
//	FUNCTION NAME:	SetupWindows()
//
//  RETURNS:				NONE
//
//  LAST MODIFIED:	PMcK	09/11/98
//
//  DESCRIPTION:    This function fills the created windows with their initial
//									default settings.
//
//	ARGUMENTS: 			NONE
//------------------------------------------------------------------------------

void SetupWindows(void)
{
  char aInitializeString[256];

  if(!errorFlag){
    // Fill Combo Boxes and Edit Boxes according to acquisition parameters
    switch(acquisitionMode){
      case 1:
        wsprintf(aInitializeString,"*SingleScan");
        break;
      case 2:
        wsprintf(aInitializeString,"*Accumulations");
        break;
      case 3:
        wsprintf(aInitializeString,"*Kinetics");
        break;
      default:
        wsprintf(aInitializeString,"DO NOT USE");
        break;
    }
    switch(readMode){
      case 0:
        strcat(aInitializeString,"*FVB");
        break;
      case 1:
        strcat(aInitializeString,"*MultiTrack");
        break;
      case 3:
        strcat(aInitializeString,"*SingleTrack");
        break;
      case 4:
        strcat(aInitializeString,"*Imaging");
        break;
      default:
        strcat(aInitializeString,"DO NOT USE");
        break;
    }

    SetWindowsToDefault(aInitializeString);

  }
  // Could not initialize
  else{
  	wsprintf(aInitializeString,"Initialization failed");
    SendMessage(ebStatus, WM_SETTEXT, 0, (LPARAM)(LPSTR)aInitializeString);
  }
}

//------------------------------------------------------------------------------
//	FUNCTION NAME:	SetWindowsToDefault()
//
//  RETURNS:				NONE
//
//  LAST MODIFIED:	PMcK	09/11/98
//
//  DESCRIPTION:    This function fills the created windows with their initial
//									default settings.
//
//	ARGUMENTS: 			Char aInitializeString: Message to be displayed in init
//																					edit box
//------------------------------------------------------------------------------

void SetWindowsToDefault(char aInitializeString[256])
{

	char aBuffer[256];
  char aBuffer2[256];
  float speed;

	// add *Open Shutter and send to window
  strcat(aInitializeString,"*Open Shutter");
  SendMessage(ebInit, WM_SETTEXT, 0, (LPARAM)(LPSTR)aInitializeString);

  // Fill in default exposure time
  wsprintf(aBuffer,"0.1");
  SendMessage(ebExposure, WM_SETTEXT, 0, (LPARAM)(LPSTR)aBuffer);

  // Fill in default threshold
  wsprintf(aBuffer,"0");
  SendMessage(ebPhotonThreshold1, WM_SETTEXT, 0, (LPARAM)(LPSTR)aBuffer);

  // Fill in default threshold
  wsprintf(aBuffer,"0");
  SendMessage(ebPhotonThreshold2, WM_SETTEXT, 0, (LPARAM)(LPSTR)aBuffer);

  // Fill in default threshold
  wsprintf(aBuffer,"0");
  SendMessage(ebPhotonThreshold3, WM_SETTEXT, 0, (LPARAM)(LPSTR)aBuffer);

  // Fill in default target temperature
  wsprintf(aBuffer,"-50");
  SendMessage(ebTemp, WM_SETTEXT, 0, (LPARAM)(LPSTR)aBuffer);

  SetShutter(1, 1, 0, 0);

  // Print Status messages
  wsprintf(aBuffer,"Head Model %s\r\n", model);
  strcat(aBuffer,"Initializing Andor MCD system\r\n");
  strcat(aBuffer,"Single Scan Selected\r\n");
  strcat(aBuffer,"Set to Image Mode\r\n");
  wsprintf(aBuffer2,"Size of CCD: %d x %d pixels\r\n",gblXPixels,gblYPixels);
  strcat(aBuffer,aBuffer2);
  GetVSSpeed(VSnumber, &speed);
  sprintf(aBuffer2,"Vertical Speed set to %g microseconds per pixel shift\r\n",speed);
  strcat(aBuffer,aBuffer2);
  GetHSSpeed(ADnumber, 0, HSnumber, &speed);
  if(caps.ulCameraType == 1)       // if using an iXon the speed is given in MHz
    sprintf(aBuffer2,"Horizontal Speed set to %g MHz\r\n",speed);
  else
    sprintf(aBuffer2,"Horizontal Speed set to %g microseconds per pixel shift\r\n",speed);
  strcat(aBuffer,aBuffer2);
  SendMessage(ebStatus, WM_SETTEXT, 0, (LPARAM)(LPSTR)aBuffer);

}

//------------------------------------------------------------------------------
//	FUNCTION NAME:	SetSystem()
//
//  RETURNS:				NONE
//
//  LAST MODIFIED:	PMcK	03/11/98
//
//  DESCRIPTION:    This function sets up the acquisition hardware settings such
//									as exposure time and starts the acquisition.
//									It also starts a timer to check when the acquisition has
//									finished (when getStatus = DRV_IDLE).
//
//	ARGUMENTS: 			NONE
//------------------------------------------------------------------------------

void SetSystem(void)
{
  float		fExposure,fAccumTime,fKineticTime;
  int 		errorValue, iThreshold;
  char 		aBuffer[256] = "";
  char 		aBuffer2[256];

  // Set Exposure Time
  GetWindowText(ebExposure,aBuffer2,5);
  fExposure=atof(aBuffer2);
  errorValue = SetExposureTime(fExposure);
  if (errorValue != DRV_SUCCESS) {
    strcat(aBuffer,"Exposure time error\r\n");
  }

  // This function only needs to be called when acquiring an image. It sets
  // the horizontal and vertical binning and the area of the image to be
  // captured. In this example it is set to 1x1 binning and is acquiring the
  // whole image
  SetImage(1,1,1,gblXPixels,1,gblYPixels);

  // It is necessary to get the actual times as the system will calculate the
  // nearest possible time. eg if you set exposure time to be 0, the system
  // will use the closest value (around 0.01s)
  GetAcquisitionTimings(&fExposure,&fAccumTime,&fKineticTime);
  strcat(aBuffer,"Actual Exposure Time is ");
  gcvt(fExposure,5,aBuffer2);
  strcat(aBuffer,aBuffer2);
  strcat(aBuffer,"\r\n");

  // Starting the acquisition also starts a timer which checks the card status
  // When the acquisition is complete the data is read from the card and
  // displayed in the paint area.
  errorValue=StartAcquisition();
  if(errorValue!=DRV_SUCCESS){
    strcat(aBuffer,"Start acquisition error\r\n");
    AbortAcquisition();
    gblData=FALSE;
  }
  else{
    strcat(aBuffer,"Starting acquisition........");
    SetTimer(hwnd,gblStatusTimer,100,NULL);   // checks 10 times per second
  }
  SendMessage(ebStatus,WM_SETTEXT,0,(LPARAM)(LPSTR)aBuffer);
  UpdateWindow(ebStatus);

}

//------------------------------------------------------------------------------
//	FUNCTION NAME:	ProcessTimer()
//
//  RETURNS:				NONE
//
//  LAST MODIFIED:	PMcK	12/11/98
//
//  DESCRIPTION:    This function handles the messages sent by the timer(s)
//
//	ARGUMENTS: 			WPARAM wparam: The timer id
//------------------------------------------------------------------------------

void ProcessTimer(WPARAM wparam)
{
	int 	status;
  int 	temperature;
  char 	aBuffer[256];
  int 	errorValue;

  // There are two timers, the getstatus timer(100) and
  // the get temp timer (200). The WM_TIMER message is
  // processed here.

  switch(wparam){
    case 100:// This case checks to see if the acquisition is complete
             // then acquires the data from the card and displays it.
      GetStatus(&status);
      if(status==DRV_IDLE){
        if(AcquireImageData()==FALSE){
          wsprintf(aBuffer,"Acquisition Error!");
          SendMessage(ebStatus,WM_SETTEXT,0,(LPARAM)(LPSTR)aBuffer);
        }
      }
      break;
    case 200:// This case displays the current temperature in the main
             // window. When the temp stabilizes at the desired level
             // the appropriate message is displayed.
      errorValue=GetTemperature(&temperature);
      switch(errorValue){
        case DRV_TEMPERATURE_STABILIZED:
          wsprintf(aBuffer,"Temperature has stabilized at %d (C)",temperature);
          break;
        case DRV_TEMPERATURE_NOT_REACHED:
          wsprintf(aBuffer,"Current temperature is %d (C)",temperature);
          break;
        default:
          wsprintf(aBuffer,"Temperature control is disabled",temperature);
          break;
      }
      SendMessage(stActTemp,WM_SETTEXT,0,(LPARAM)(LPSTR)aBuffer);
  }
}

//------------------------------------------------------------------------------
//	FUNCTION NAME:	ProcessPushButtons()
//
//  RETURNS:				NONE
//
//  LAST MODIFIED:	PMcK	12/11/98
//
//  DESCRIPTION:    This function handles the messages sent by the pushbuttons
//
//	ARGUMENTS: 			LPARAM lparam: The button id
//------------------------------------------------------------------------------

void ProcessPushButtons(LPARAM lparam)
{
	int 	 temperature;
  int 	 errorValue;
  char 	 aBuffer[256];
  int 	 status;
  int i;

	if(lparam==(LPARAM)pbStart){  // Start acquisition button is pressed
    gblData=TRUE;               // tells system an acq has taken place
    GetStatus(&status);
    KillTimer(hwnd,gblTempTimer);// stop temp display while acquiring
    if(status==DRV_IDLE){
      SetSystem();              // Set hardware and start acquisition
      FillRectangle();          // clear window ready for data trace
    }
  }

  if(lparam==(LPARAM)pbAbort){
    // abort acquisition if in progress
    GetStatus(&status);
    if(status==DRV_ACQUIRING){
      errorValue=AbortAcquisition();
      if(errorValue!=DRV_SUCCESS){
        wsprintf(aBuffer,"Error aborting acquistion");
        SendMessage(ebStatus,WM_SETTEXT,0,(LPARAM)(LPSTR)aBuffer);
      }
      else{
        wsprintf(aBuffer,"Aborting Acquisition....");
        SendMessage(ebStatus,WM_SETTEXT,0,(LPARAM)(LPSTR)aBuffer);
      }
      gblData=FALSE;    // tell system no acq data in place
    }
    // or else let user know none is in progress
    else{
      wsprintf(aBuffer,"System not Acquiring");
      SendMessage(ebStatus,WM_SETTEXT,0,(LPARAM)(LPSTR)aBuffer);
    }
  }

  if(lparam==(LPARAM)pbPostProcess){
    if(status!=DRV_ACQUIRING){
      if (pImageArray) {
        char aBuffer2[256];
        int iNumThresholds = 0, i;
        float fPhotonThresholdList[3];

        GetWindowText(ebPhotonThreshold1,aBuffer2,5);
        fPhotonThresholdList[0]=atoi(aBuffer2);

        GetWindowText(ebPhotonThreshold2,aBuffer2,5);
        fPhotonThresholdList[1]=atoi(aBuffer2);

        GetWindowText(ebPhotonThreshold3,aBuffer2,5);
        fPhotonThresholdList[2]=atoi(aBuffer2);

        for (i = 0; i < 3; i++) {
          if (fPhotonThresholdList[i] > 0) {
            iNumThresholds++;
          }
        }

        pOutputImage = malloc((gblYPixels*gblXPixels)*sizeof(long));
        errorValue = PostProcessPhotonCounting(pImageArray, pOutputImage, (gblYPixels*gblXPixels), 1, 1, iNumThresholds, &fPhotonThresholdList[0], gblYPixels, gblXPixels);
        if (DRV_SUCCESS == errorValue) {
          // Find max value and scale data to fill rect
          long minValue, maxValue;
          pImageArray = &pOutputImage[0];
          FillRectangle();
          if(DrawLines(&maxValue,&minValue)==FALSE){
            char aBuffer[20];
            KillTimer(hwnd,gblStatusTimer);
            wsprintf(aBuffer, "Data range is zero");
            SendMessage(ebStatus,WM_SETTEXT,0,(LPARAM)(LPSTR)aBuffer);
          }
        }

      }
      else {
        wsprintf(aBuffer,"No Image available to Post Process");
        SendMessage(ebStatus,WM_SETTEXT,0,(LPARAM)(LPSTR)aBuffer);
      }
    }
    else {
      wsprintf(aBuffer,"Cannot Post Process during Acquisition");
      SendMessage(ebStatus,WM_SETTEXT,0,(LPARAM)(LPSTR)aBuffer);
    }
  }

  // This section responds to a click on the Close button. It is
  // vital to raise the temperature to above 0 before closing
  // to reduce damage to the head. This routine simply blocks exiting
  // the program until the temp is above 0
  if(lparam==(LPARAM)pbClose){  // close application
    GetTemperature(&temperature);
    if(temperature<0){
      if(gblCooler==TRUE){
        wsprintf(aBuffer,"5");
        SendMessage(ebTemp,WM_SETTEXT,0,(LPARAM)(LPSTR)aBuffer);
        SwitchCoolerOn();
      }
      wsprintf(aBuffer,"Raising temperature to above zero (C)");
      strcat(aBuffer,"\r\nPress close again when temp is above zero");
      SendMessage(ebStatus,WM_SETTEXT,0,(LPARAM)(LPSTR)aBuffer);
    }
    else{
      KillTimer(hwnd,gblTempTimer);
      DestroyWindow(hwnd);
    }
  }

  if(lparam==(LPARAM)pbCoolerOn)    // switch on cooler and set temperature

    SwitchCoolerOn();     					// Proceedure to initiate cooling

  if(lparam==(LPARAM)pbCoolerOff)   // switch off cooler

    SwitchCoolerOff();    					// Proceedure to stop cooling
}

//------------------------------------------------------------------------------
//	FUNCTION NAME:	SwitchCoolerOn()
//
//  RETURNS:				NONE
//
//  LAST MODIFIED:	PMcK	03/11/98
//
//  DESCRIPTION:    This function switches on the cooler and sets the
//									temperature according to the value in the edit box. It also
//									checks to see if the requested temperature is in the valid
//									range returned by GetTemperatureRange.
//
//	ARGUMENTS: 			NONE
//------------------------------------------------------------------------------

void SwitchCoolerOn(void)
{
  char 		aBuffer[256];
  char 		aBuffer2[256];
  int 		MinTemp,MaxTemp,temperature;
  int 		errorValue;

  wsprintf(aBuffer,"");  			// clear buffer

  // Get temp from edit box
  GetWindowText(ebTemp,aBuffer2,5);
  temperature=atoi(aBuffer2);

  // check if temp is in valid range
  errorValue=GetTemperatureRange(&MinTemp,&MaxTemp);
  if(errorValue!=DRV_SUCCESS)
    strcat(aBuffer,"Temperature error");
  else{
    if(temperature<MinTemp||temperature>MaxTemp)
      strcat(aBuffer,"Temperature is out of range");
    else{
      // if it is in range, switch on cooler and set temp
      errorValue=CoolerON();
      if(errorValue!=DRV_SUCCESS){
        wsprintf(aBuffer,"Could not switch cooler on");
      }
      else{
        gblCooler=TRUE;
        SetTimer(hwnd,gblTempTimer,1000,NULL);
        errorValue=SetTemperature(temperature);
        if(errorValue!=DRV_SUCCESS){
          strcat(aBuffer,"Could not set temperature");
        }
        else
          wsprintf(aBuffer2,"Temperature has been set to %d (C)",temperature);
        strcat(aBuffer,aBuffer2);
      }
    }
  }
  SendMessage(ebStatus,WM_SETTEXT,0,(LPARAM)(LPSTR)aBuffer);
}

//------------------------------------------------------------------------------
//	FUNCTION NAME:	SwitchCoolerOff()
//
//  RETURNS:				NONE
//
//  LAST MODIFIED:	PMcK	03/11/98
//
//  DESCRIPTION:    This function switches the cooler off
//
//	ARGUMENTS: 			NONE
//------------------------------------------------------------------------------

void SwitchCoolerOff(void)
{
  int 		errorValue;
  char 		aBuffer[256];

  KillTimer(hwnd,gblTempTimer);
  errorValue=CoolerOFF();
  if(errorValue!=DRV_SUCCESS){
    wsprintf(aBuffer,"Could not switch cooler off");
    SendMessage(ebStatus,WM_SETTEXT,0,(LPARAM)(LPSTR)aBuffer);
  }
  else{
    gblCooler=FALSE;
    wsprintf(aBuffer,"Temperature control is disabled");
    SendMessage(stActTemp,WM_SETTEXT,0,(LPARAM)(LPSTR)aBuffer);
    wsprintf(aBuffer,"Temperature control is disabled");
    SendMessage(ebStatus,WM_SETTEXT,0,(LPARAM)(LPSTR)aBuffer);
  }
}


//------------------------------------------------------------------------------
//	FUNCTION NAME:	UpdateDialogWindows()
//
//  RETURNS:				NONE
//
//  LAST MODIFIED:	PMcK	03/11/98
//
//  DESCRIPTION:    This function updates the individual windows and is called
//									from inside the WM_PAINT message. This ensures that the
//									windows are present when the main window is re-drawn.
//
//	ARGUMENTS: 			NONE
//------------------------------------------------------------------------------

void UpdateDialogWindows(void)
{
  UpdateWindow(ebInit);
  UpdateWindow(stInit);
  UpdateWindow(ebExposure);
  UpdateWindow(stExposure);
  UpdateWindow(stPhotonThreshold);
  UpdateWindow(ebPhotonThreshold1);
  UpdateWindow(stPhotonThreshold1);
  UpdateWindow(ebPhotonThreshold2);
  UpdateWindow(stPhotonThreshold2);
  UpdateWindow(ebPhotonThreshold3);
  UpdateWindow(stPhotonThreshold3);
  UpdateWindow(ebTemp);
  UpdateWindow(stTemp);
  UpdateWindow(pbCoolerOn);
  UpdateWindow(pbCoolerOff);
  UpdateWindow(stActTemp);
  UpdateWindow(ebStatus);
  UpdateWindow(stStatus);
  UpdateWindow(pbStart);
  UpdateWindow(pbAbort);
  UpdateWindow(pbClose);
  UpdateWindow(pbPostProcess);
}

//------------------------------------------------------------------------------
//	FUNCTION NAME:	FillRectangle()
//
//  RETURNS:				NONE
//
//  LAST MODIFIED:	PMcK	03/11/98
//
//  DESCRIPTION:    This function paints a white rectangle onto which we paint
//									the data trace.
//
//	ARGUMENTS: 			NONE
//------------------------------------------------------------------------------

void FillRectangle(void)
{
  HGDIOBJ 	prevObject;
  HBRUSH 		fill;
  HDC 			hdcRect;
  RECT 			dummy;

  rect.left=340;
  rect.top=20;
  rect.right=611;
  rect.bottom=291;
  dummy = rect;    // FillRect excludes the right and bottom borders
  dummy.right++;   // dummy paints over those too
  dummy.bottom++;

  hdcRect=GetDC(hwnd);
  fill=CreateSolidBrush(0xFFFFFF);        // Select white brush
  prevObject=SelectObject(hdcRect,fill);
  FillRect(hdcRect,&dummy,fill);           // Paint white rect
 SelectObject(hdcRect,prevObject);
  DeleteObject(fill);
  ReleaseDC(hwnd,hdcRect);
}

//------------------------------------------------------------------------------
//	FUNCTION NAME:	AcquireImageData()
//
//  RETURNS:				TRUE: Image data acquired and displayed successfully
//									FALSE: Error acquiring or displaying data
//
//  LAST MODIFIED:	PMcK	03/11/98
//
//  DESCRIPTION:    This function gets the acquired data from the card and
//									stores it in the global buffer pImageArray. It is called
//									from WM_TIMER after the acquisition is complete and goes on
//									to display the data	using DrawLines() and kill the timer.
//
//	ARGUMENTS: 			NONE
//------------------------------------------------------------------------------

BOOL AcquireImageData(void)
{
  int 	size;
  int 	errorValue;
  char 	aBuffer[256];
  char 	aBuffer2[256];
  long 	MaxValue;
  long  MinValue;

  if(!gblData){                             // If there is no data the acq has
    wsprintf(aBuffer,"Acquisition aborted!"); // been aborted
    SendMessage(ebStatus,WM_SETTEXT,0,(LPARAM)(LPSTR)aBuffer);
    KillTimer(hwnd,gblStatusTimer);
    return TRUE;
  }

  size=AllocateBuffers();  // Allocate memory for image data. Size is returned
                           // for GetAcquiredData which needs the buffer size

  errorValue=GetAcquiredData(pImageArray,size);
  if(errorValue!=DRV_SUCCESS){
    wsprintf(aBuffer,"Acquisition error!");
    SendMessage(ebStatus,WM_SETTEXT,0,(LPARAM)(LPSTR)aBuffer);
    return FALSE;
  }

  // Display data and query max data value to be displayed in status box
  FillRectangle();
  if(DrawLines(&MaxValue,&MinValue)==FALSE){
  	char aBuffer[20];
    KillTimer(hwnd,gblStatusTimer);
    wsprintf(aBuffer, "Data range is zero");
   	SendMessage(ebStatus,WM_SETTEXT,0,(LPARAM)(LPSTR)aBuffer);         
    return FALSE;
  }

  KillTimer(hwnd,gblStatusTimer);           // kill status timer
  if(gblCooler)
    SetTimer(hwnd,gblTempTimer,1000,NULL);  // start temp timer again when acq
                                            //	is complete

  // tell user acquisition is complete
  wsprintf(aBuffer,"Acquisition complete !\r\n");
  strcat(aBuffer,"Image taken\r\n");
  wsprintf(aBuffer2,"Max data value is %d counts\r\n",MaxValue);
  strcat(aBuffer,aBuffer2);
  wsprintf(aBuffer2,"Min data value is %d counts",MinValue);
  strcat(aBuffer,aBuffer2);
  SendMessage(ebStatus,WM_SETTEXT,0,(LPARAM)(LPSTR)aBuffer);
  return TRUE;
}

//------------------------------------------------------------------------------
//	FUNCTION NAME:	PaintDataWindow()
//
//  RETURNS:				NONE
//
//  LAST MODIFIED:	PMcK	12/11/98
//
//  DESCRIPTION:    This function handles the WM_PAINT messages sent by the
//									application. The WM_PAINT message repaints the screen when
//									the application opens and when you switch between
//									applications. When the app opens for the first time paint a
//									logo onto the paint area. When data is acquired paint it
//									instead.
//
//	ARGUMENTS: 			NONE
//------------------------------------------------------------------------------

void PaintDataWindow(void)
{
			HANDLE 				hBmp;           // handle to Andor bitmap
      HDC 					hBitmapDC;
      HDC						hMemDC;
      PAINTSTRUCT 	PtrStr;
      long 					MaxValue;
      long 					MinValue;
      int						bitmapWidth=266;
      int						bitmapHeight=64;

      // Redraw all dialog elements
      UpdateDialogWindows();        // Control windows
      FillRectangle();              // Paint area

      // Paint bitmap onto screen until first acquisition is taken
      if(!gblData || pImageArray==NULL){
        hBmp=LoadBitmap(hInst,"Andortch");
        hBitmapDC=BeginPaint(hwnd,&PtrStr);
        hMemDC=CreateCompatibleDC(hBitmapDC);
        SelectObject(hMemDC,hBmp);

        //Place Bitmap in center of paint area
        BitBlt(hBitmapDC,
               rect.left+(((rect.right-rect.left)-266)/2),   // x
               rect.top+(((rect.bottom-rect.top)-66)/2),     // y
               bitmapWidth,                                  // width
               bitmapHeight,                                 // height
               hMemDC,0,0,SRCCOPY);
        DeleteDC(hMemDC);
        EndPaint(hwnd,&PtrStr);
      }
      // When data is available paint it onto the screen using drawlines()
      else{
        if(DrawLines(&MaxValue,&MinValue)==FALSE){  // values are not used in this case
          char aBuffer[20];
        	wsprintf(aBuffer, "Data range is zero");
	      	SendMessage(ebStatus,WM_SETTEXT,0,(LPARAM)(LPSTR)aBuffer);
        }
      }

      // tell system that window is redrawn
      ValidateRect(hwnd,NULL);
}

//------------------------------------------------------------------------------
//	FUNCTION NAME:	DrawLines()
//
//  RETURNS:				TRUE: Function succeeded
//									FALSE: Data failed to display
//
//  LAST MODIFIED:	Elm	18/10/04
//
//  DESCRIPTION:    This function displays the data onto the screen using
//									PaintImage().
//
//	ARGUMENTS: 			int scanNo:             track to be displayed
//									long *ppMaxDataValue:   This returns the max value to be
//																					displayed in the status box.
//------------------------------------------------------------------------------

BOOL DrawLines(long *pMaxDataValue,long *pMinDataValue)
{
  int 		i;
  BOOL 		bRetValue=TRUE;
  long 		maxValue=1;
  long		minValue=65536;

  if(gblData && pImageArray!=NULL){

    // Find max value and scale data to fill rect
    for(i=0;i<(gblXPixels*gblYPixels);i++){
      if(pImageArray[i]>maxValue)
        maxValue=pImageArray[i];
      if(pImageArray[i]<minValue)
        minValue=pImageArray[i];
    }

    if(maxValue == minValue)
    	return FALSE;

    PaintImage(maxValue, minValue, 0); //Display image

    *pMaxDataValue=maxValue;    // tell acquiredata function the max value so
                                // that it can display it in the status box
    *pMinDataValue=minValue;    // tell acquiredata function the min value so
                                // that it can display it in the status box
  }
  else
  	bRetValue=FALSE;
  return bRetValue;
}

//------------------------------------------------------------------------------
//	FUNCTION NAME:	AllocateBuffers()
//
//  RETURNS:				int size:  size of the image buffer
//
//  LAST MODIFIED:	PMcK	03/11/98
//
//  DESCRIPTION:    This function allocates enough memory for the buffers(if not
//									allocated already).
//
//	ARGUMENTS: 			NONE
//------------------------------------------------------------------------------

int AllocateBuffers(void)
{

	int size;

	FreeBuffers();

	size=gblXPixels*gblYPixels;  // Needs to hold full image

  // only allocate if necessary
	if(!pImageArray)
  	pImageArray=malloc(size*sizeof(long));

  return size;
}

//------------------------------------------------------------------------------
//	FUNCTION NAME:	FreeBuffers()
//
//  RETURNS:				NONE
//
//  LAST MODIFIED:	PMcK	03/11/98
//
//  DESCRIPTION:    This function frees the memory allocated to each buffer and
//									is called when the application exits.
//
//	ARGUMENTS: 			NONE
//------------------------------------------------------------------------------

void FreeBuffers(void)
{
  // free all allocated memory
  if(pImageArray){
    free(pImageArray);
    pImageArray = NULL;
  }
}

//------------------------------------------------------------------------------
//	FUNCTION NAME:	PaintImage()
//
//  RETURNS:				NONE
//
//  LAST MODIFIED:	Elm   18/10/04
//
//  DESCRIPTION:    This function plots the data onto a bitmap and then displays
//                  it. Data is scaled from minValue to maxValue.
//
//	ARGUMENTS: 			long maxValue: Max data value
//                  long minValue: Min data value
//                  int Start:     image to be displayed from pImageArray
//------------------------------------------------------------------------------

void PaintImage(long maxValue, long minValue, int Start)
{
  HDC hDC;
  float xscale, yscale, zscale, modrange;
  double dTemp;
  int width, height;
  int i, j, x, z, iTemp;
	HANDLE hloc;
	PBITMAPINFO pbmi;
	WORD argbq[Color];
  BYTE *DataArray;
  BOOL lvRunning = FALSE;

  SystemParametersInfo(SPI_GETSCREENSAVERRUNNING, 0, &lvRunning, 0);
  if (lvRunning == FALSE) {// if the screensaver is not running, plot data.
    hDC = GetDC(hwnd);
    CreateIdentityPalette(hDC);

    modrange     = maxValue - minValue;
    width        = rect.right - rect.left + 1;
    if(width%4)                 // width must be a multiple of 4,
      width += (4-width%4);     // otherwise StretchDIBits has problems
    height       = rect.bottom - rect.top + 1;
    xscale       = (float)(gblXPixels)/(float)(width);
    yscale       = (256.0)/(float)modrange;
    zscale       = (float)(gblYPixels)/(float)(height);

    for (i = 0; i < Color; i++) argbq[i] = (WORD)i;

    hloc = LocalAlloc(LMEM_ZEROINIT | LMEM_MOVEABLE,
      sizeof(BITMAPINFOHEADER) + (sizeof(WORD)*Color));

    pbmi = (PBITMAPINFO) LocalLock(hloc);
    pbmi->bmiHeader.biSize        = sizeof(BITMAPINFOHEADER);
    pbmi->bmiHeader.biPlanes      = 1;
    pbmi->bmiHeader.biBitCount    = 8;
    pbmi->bmiHeader.biCompression = BI_RGB;
    pbmi->bmiHeader.biClrUsed     = Color;
    pbmi->bmiHeader.biWidth       = width;
    pbmi->bmiHeader.biHeight      = height;
    memcpy(pbmi->bmiColors, argbq, sizeof(WORD) * Color);

    DataArray = (BYTE*)malloc(width * height * sizeof(BYTE));
    memset(DataArray, 255, width * height);

    for (i = 0; i < height; i++) {
      z = (int)(i * zscale);
      for (j = 0; j < width; j++) {
        x = (int)(j * xscale);
        dTemp = ceil(yscale * (pImageArray[Start + x + z * gblXPixels] - minValue));
        if (dTemp < 0) iTemp = 0;
        else if (dTemp > Color - 1) iTemp = Color - 1;
        else iTemp = (int)dTemp;
        DataArray[j + i * width] = (BYTE)iTemp;
      }
    }

    SetStretchBltMode(hDC,COLORONCOLOR);
    StretchDIBits(hDC, rect.left, rect.top, width, height, 0, 0, width, height,
    DataArray, (BITMAPINFO FAR*)pbmi, DIB_PAL_COLORS, SRCCOPY);

    free(DataArray);
    ReleaseDC(hwnd, hDC);
    LocalUnlock(hloc);
    LocalFree(hloc);
  }
}

//------------------------------------------------------------------------------
//	FUNCTION NAME:	CreateIdentityPalette()
//
//  RETURNS:				NONE
//
//  LAST MODIFIED:	PMcK	03/11/98
//
//  DESCRIPTION:    This function creates the color palette used to display the
//                  data.
//
//	ARGUMENTS: 			NONE
//------------------------------------------------------------------------------

void CreateIdentityPalette(HDC ScreenDC)
{
  HPALETTE AppPalette;
  struct {
	 WORD Version;
	 WORD NumberEntries;
	 PALETTEENTRY aEntries[256];
  } Palette = { 0x300, 256};
  int i;

  GetSystemPaletteEntries(ScreenDC, 0, 256, Palette.aEntries);

  for (i = 0; i < Color; i++) {
		Palette.aEntries[i].peRed = LOBYTE(i);
		Palette.aEntries[i].peGreen = LOBYTE(i);
		Palette.aEntries[i].peBlue = LOBYTE(i);
    Palette.aEntries[i].peFlags = PC_RESERVED;
  }

  AppPalette = CreatePalette((LOGPALETTE *)&Palette);
  SelectPalette(ScreenDC, AppPalette, TRUE);
  RealizePalette(ScreenDC);
}

