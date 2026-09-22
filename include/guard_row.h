/*
 * guard_row.h
 *
 *  Created on: 18 Sept 2026
 *      Author: billy
 */

#ifndef SERVICES_FIELD_TRIALS_INCLUDE_GUARD_ROW_H_
#define SERVICES_FIELD_TRIALS_INCLUDE_GUARD_ROW_H_


#include "row.h"


typedef struct GuardRow
{
	Row gu_base;
} GuardRow;


#ifdef __cplusplus
extern "C"
{
#endif


DFW_FIELD_TRIAL_SERVICE_LOCAL Row *AllocateGuardRow (bson_oid_t *id_p, const uint32 study_index, Plot *parent_plot_p);


DFW_FIELD_TRIAL_SERVICE_LOCAL GuardRow *GetGuardRowFromJSON (const json_t *row_json_p, Plot *plot_p, const Study *study_p, const ViewFormat format, FieldTrialServiceData *data_p);

#ifdef __cplusplus
}
#endif



#endif /* SERVICES_FIELD_TRIALS_INCLUDE_GUARD_ROW_H_ */
