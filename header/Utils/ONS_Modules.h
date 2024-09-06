// ---------------------------------------------------------------------------
//  Filename:  ONS_Modules.h
//  Created by: Shmulik Hass
//  Date:  15.12.15
//  Orisol (c) 
// ---------------------------------------------------------------------------

#ifndef _ONS_MODULES_H_
#define _ONS_MODULES_H_

/* OBSTACLES */

#define  OBSTACLES_MODULE  1000
#define	 OBS_ERROR(err)  (OBSTACLES_MODULE+(err))

/* SEWPGS */

#define  SEWPGS_MODULE  2000
#define	 SEWPGS_ERROR(err)  (SEWPGS_MODULE+(err))

/* SEWPGM */

#define  SEWPGM_MODULE  3000
#define	 SEWPGM_ERROR(err)  (SEWPGM_MODULE+(err))

/* INI_FILE */

#define  INIFILE_MODULE  4000
#define	 INIFILE_ERROR(err)  (INIFILE_MODULE+(err))


/* JOBDATA */

#define  JOBDATA_MODULE  5000
#define	 JOBDATA_ERROR(err)  (JOBDATA_MODULE+(err))

/* FILEPATH */

#define  FILEPATH_MODULE  6000
#define	 FILEPATH_ERROR(err)  (JOBDATA_MODULE+(err))

/* RFIDJOBS*/

#define  RFIDJOBS_MODULE  6500
#define	 RFIDJOBS_ERROR(err)  (RFIDJOBS_MODULE+(err))


/* Loader */

#define  LOADER_MODULE  7000
#define	 LOADER_ERROR(err)  (LOADER_MODULE+(err))


// Loader CONTROL */
#define  LOADER_CTRL_MODULE	8000
#define	LOADER_CTRL_ERROR(err)  (LOADER_CTRL_MODULE+(err))


/* CONTROL */
#define  CTRL_MODULE	9000
#define	 CTRL_ERROR(err)  (CTRL_MODULE+(err))

//USBCOMM	10000  - save for the module

/* CALIBRATION */
#define  CLIB_MODULE	11000
#define	 UW_CLIB_MODULE	11100
#define	 CLIB_ERROR(err)  (CLIB_MODULE+(err))
#define	 UW_CLIB_ERROR(err)  (UW_CLIB_MODULE+(err))


/* Network Mngr */
#define  NETWORK_MNGR_MODULE	12000
#define	 NETWORK_MNGR_ERROR(err)  (NETWORK_MNGR_MODULE+(err))

/* iStitching Mngr */
#define  IST_MNGR_MODULE	13000
#define	 IST_MNGR_ERROR(err)  (IST_MNGR_MODULE+(err))
#endif

