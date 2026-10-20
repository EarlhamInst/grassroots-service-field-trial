/*
 * image_importer.c
 *
 *  Created on: 17 Oct 2026
 *      Author: tyrrells
 */


#include "image_importer.h"

#include "dfw_util.h"
#include "study.h"

/*
 *
 * Get list of files
 *
 * Parse each filename to get the Study and Plot number
 *
 * Collate all of the study's images together so that we don't
 * repeatedly get the same Study
 *
 * Add images
 *
 *
 */




void ImportImages (const char * const code_s, const ViewFormat format, const FieldTrialServiceData * const data_p)
{
	Study *study_p = GetDFWObjectByNamedStringField (ST_CODE_S, code_s, DFTD_STUDY, GetStudyCallback, format, data_p);

}



OperationStatus ImportImage (Study *study_p, const ViewFormat format, const FieldTrialServiceData * const data_p)
{
	OperationStatus status = OS_FAILED;


	return status;
}
