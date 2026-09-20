/*
 * guard_row.c
 *
 *  Created on: 18 Sept 2026
 *      Author: billy
 */


#include "guard_row.h"


static bool AddGuardRowToJSON (const Row *row_p, json_t *row_json_p, const ViewFormat format, const FieldTrialServiceData *data_p);

static bool AddGuardRowToFD (const Row *row_p, json_t *row_fd_p, const FieldTrialServiceData *service_data_p, const char * const null_sequence_s);



Row *AllocateGuardRow (bson_oid_t *id_p, const uint32 study_index, Plot *parent_plot_p)
{
	GuardRow *row_p = (GuardRow *) AllocMemory (sizeof (GuardRow));

	if (row_p)
		{
			if (InitRow (& (row_p -> gu_base), id_p, study_index, parent_plot_p, RT_GUARD, NULL, AddGuardRowToJSON, NULL, AddGuardRowToFD))
				{
					return (& (row_p -> gu_base));
				}

			FreeMemory (row_p);
		}

	return NULL;
}



GuardRow *GetGuardRowFromJSON (const json_t *row_json_p, Plot *plot_p, const Study *study_p, const ViewFormat format, FieldTrialServiceData *data_p)
{
	GuardRow *row_p = (GuardRow *) AllocMemory (sizeof (GuardRow));

	if (row_p)
		{
			SetRowCallbackFunctions (& (row_p -> gu_base),
															 NULL,
															 AddGuardRowToJSON,
															 NULL,
															 AddGuardRowToFD);

			if (PopulateRowFromJSON (& (row_p -> gu_base), plot_p, row_json_p, format, data_p))
				{
					return row_p;
				}

			FreeMemory (row_p);
		}

	return NULL;
}




static bool AddGuardRowToJSON (const Row *row_p, json_t *row_json_p, const ViewFormat format, const FieldTrialServiceData *data_p)
{
	GuardRow *guard_row_p = (GuardRow *) row_p;
	bool success_flag = false;

	if (SetJSONBoolean (row_json_p, RO_GUARD_S, true))
		{
			success_flag = true;
		}
	else
		{
			PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, row_json_p, "Failed to add \"%s\": true", RO_GUARD_S);
		}

	return success_flag;
}


static bool AddGuardRowToFD (const Row *row_p, json_t *row_fd_p, const FieldTrialServiceData *service_data_p, const char * const null_sequence_s)
{
	GuardRow *guard_row_p = (GuardRow *) row_p;
	bool success_flag = false;

	if (SetJSONBoolean (row_fd_p, RO_GUARD_S, true))
		{
			success_flag = true;
		}
	else
		{
			PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, row_fd_p, "Failed to add \"%s\": true", RO_GUARD_S);
		}

	return success_flag;

}

