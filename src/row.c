/*
 * base_row.c
 *
 *  Created on: 29 Jun 2022
 *      Author: billy
 */

#define ALLOCATE_BASE_ROW_TAGS (1)
#include "row.h"

#include "memory_allocations.h"
#include "streams.h"
#include "mongodb_tool.h"
#include "plot.h"

#include "blank_row.h"
#include "discard_row.h"
#include "guard_row.h"
#include "standard_row.h"
#include "plot_jobs.h"


static const char *S_ROW_TYPES_SS [RT_NUM_VALUES] = { "Standard", "Discard", "Blank", "Guard" };



Row *AllocateBaseRow (bson_oid_t *id_p, const uint32 study_index, Plot *parent_plot_p, RowType rt,
											void (*clear_fn) (Row *row_p),
											bool (*add_to_json_fn) (const Row *row_p, json_t *row_json_p, const ViewFormat format, const FieldTrialServiceData *data_p),
											bool (*add_from_json_fn) (Row *row_p, const json_t *row_json_p, const FieldTrialServiceData *data_p),
											bool (*add_to_fd_fn) (const Row *row_p, json_t *row_fd_p, const FieldTrialServiceData *service_data_p, const char * const null_sequence_s))
{
	Row *row_p = (Row *) AllocMemory (sizeof (Row));

	if (row_p)
		{
			if (InitRow (row_p, id_p, study_index, parent_plot_p, rt, clear_fn, add_to_json_fn, add_from_json_fn, add_to_fd_fn))
				{
					return row_p;
				}

			FreeMemory (row_p);
		}

	return NULL;
}


bool InitRow (Row *row_p, bson_oid_t *id_p, const uint32 study_index, Plot *parent_plot_p, RowType rt,
							void (*clear_fn) (Row *row_p),
							bool (*add_to_json_fn) (const Row *row_p, json_t *row_json_p, const ViewFormat format, const FieldTrialServiceData *data_p),
							bool (*add_from_json_fn) (Row *row_p, const json_t *row_json_p, const FieldTrialServiceData *data_p),
							bool (*add_to_fd_fn) (const Row *row_p, json_t *row_fd_p, const FieldTrialServiceData *service_data_p, const char * const null_sequence_s))

{
	if (!id_p)
		{
			id_p = GetNewBSONOid ();

			if (!id_p)
				{
					PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to allocate BSON oid for row at [" UINT32_FMT ", " UINT32_FMT "] for study \"%s\"", parent_plot_p -> pl_parent_p -> st_name_s);
					return false;
				}
		}

	row_p -> ro_type = rt;
	row_p -> ro_id_p = id_p;
	row_p -> ro_by_study_index = study_index;
	row_p -> ro_plot_p = parent_plot_p;
	row_p -> ro_study_p = parent_plot_p -> pl_parent_p;

	SetRowCallbackFunctions (row_p, clear_fn, add_to_json_fn, add_from_json_fn, add_to_fd_fn);

	return true;
}


void ClearRow (Row *row_p)
{
	FreeBSONOid (row_p -> ro_id_p);
}


void FreeRow (Row *row_p)
{
	ClearRow (row_p);

	if (row_p -> ro_clear_fn)
		{
			row_p -> ro_clear_fn (row_p);
		}

	FreeMemory (row_p);
}



void SetRowType (Row *row_p, RowType rt)
{
	row_p -> ro_type = rt;
}


RowType GetRowType (const Row *row_p)
{
	return (row_p -> ro_type);
}



bool AddRowToJSON (const Row *row_p, json_t *row_json_p, const ViewFormat format, const FieldTrialServiceData *data_p)
{
	bool success_flag = false;

	if (SetJSONInteger (row_json_p, RO_STUDY_INDEX_S, row_p -> ro_by_study_index))
		{
			if (AddCompoundIdToJSON (row_json_p, row_p -> ro_id_p))
				{
					/*
					 * We only need to store the parent plot id if the JSON is for the backend
					 */
					if (format == VF_STORAGE)
						{
							if (AddNamedCompoundIdToJSON (row_json_p, row_p -> ro_plot_p -> pl_id_p, RO_PLOT_ID_S))
								{
									if (AddNamedCompoundIdToJSON (row_json_p, row_p -> ro_study_p -> st_id_p, RO_STUDY_ID_S))
										{
											const char *type_s = GetRowTypeAsString (row_p -> ro_type);

											if (type_s)
												{
													if (SetJSONString (row_json_p, RO_ROW_TYPE_S, type_s))
														{
															if (row_p -> ro_add_to_json_fn)
																{
																	success_flag = row_p -> ro_add_to_json_fn (row_p, row_json_p, format, data_p);
																}
															else
																{
																	success_flag = true;
																}
														}		/* if (SetJSONString (row_json_p, BR_ROW_TYPE_S, type_s)) */
													else
														{
															PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, row_json_p, "SetJSONString() failed for row " UINT32_FMT " in study \"%s\", \"%s\": \"%s\"",
																								 row_p -> ro_by_study_index, row_p -> ro_plot_p -> pl_parent_p -> st_name_s,
																								 RO_ROW_TYPE_S, type_s);
														}

												}		/* if (type_s) */
											else
												{
													PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "GetRowTypeAsString () failed for row " UINT32_FMT " in study \"%s\" with type %d",
																			 row_p -> ro_by_study_index, row_p -> ro_plot_p -> pl_parent_p -> st_name_s, row_p -> ro_type);
												}

										}		/* if (AddNamedCompoundIdToJSON (row_json_p, row_p -> ro_plot_p -> pl_id_p, RO_PLOT_ID_S)) */
									else
										{
											PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, row_json_p, "Failed to add study id for row " UINT32_FMT " in study \"%s\"",
																				 row_p -> ro_by_study_index, row_p -> ro_plot_p -> pl_parent_p -> st_name_s);
										}

								}		/* if (AddNamedCompoundIdToJSON (row_json_p, row_p -> ro_plot_p -> pl_id_p, RO_PLOT_ID_S)) */
							else
								{
									PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, row_json_p, "Failed to add plot id for for row " UINT32_FMT " in study \"%s\"",
																		 row_p -> ro_by_study_index, row_p -> ro_plot_p -> pl_parent_p -> st_name_s);
								}


						}		/* if (format == VF_STORAGE) */
					else
						{
							success_flag = true;
						}

				}		/* if (AddCompoundIdToJSON (row_json_p, row_p -> ro_id_p)) */
			else
				{
					char *id_s = GetBSONOidAsString (row_p -> ro_id_p);

					if (id_s)
						{
							PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, row_json_p, "Failed to add row compound id \"%s\"", id_s);
							FreeBSONOidString (id_s);
						}
					else
						{
							PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, row_json_p, "Failed to row add compound id");
						}
				}

		}		/* if (SetJSONInteger (row_json_p, RO_STUDY_INDEX_S, row_p -> ro_by_study_index)) */
	else
		{
			PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, row_json_p, "Failed to add \"%s\": " UINT32_FMT, RO_STUDY_INDEX_S, row_p -> ro_by_study_index);
		}

	return success_flag;
}


json_t *GetRowAsJSON (const Row *row_p, const ViewFormat format, JSONProcessor *processor_p, const FieldTrialServiceData *data_p)
{
	json_t *row_json_p = json_object ();

	if (row_json_p)
		{
			if (AddRowToJSON (row_p, row_json_p, format, data_p))
				{
					bool success_flag = true;

					if (row_p -> ro_add_to_json_fn)
						{
							success_flag = row_p -> ro_add_to_json_fn (row_p, row_json_p, format, data_p);
						}

					if (success_flag)
						{
							return row_json_p;
						}

				}		/* if (AddBaseRowToJSON (& (row_p -> sr_base), row_json_p, format)) */
			else
				{
					PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "AddRowToJSON () failed for row " UINT32_FMT " in \"%s\"", row_p -> ro_by_study_index, row_p -> ro_study_p -> st_name_s);
				}

			json_decref (row_json_p);
		}		/* if (row_json_p) */

	return NULL;
}






Row *GetRowFromJSON (const json_t *row_json_p, Plot *plot_p, const Study *study_p, const ViewFormat format, FieldTrialServiceData *data_p)
{
	Row *row_p = NULL;
	RowType rt = RT_STANDARD;
	const char *type_s = GetJSONString (row_json_p, RO_ROW_TYPE_S);

	if (type_s)
		{
			if (!SetRowTypeFromString (&rt, type_s))
				{
					PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, row_json_p, "Unknown row type \"%s\"", type_s);
				}

		}		/* if (type_s) */

	switch (rt)
	{
		case RT_STANDARD:
			{
				StandardRow *sr_p = GetStandardRowFromJSON (row_json_p, plot_p, NULL, study_p, format, data_p);

				if (sr_p)
					{
						row_p = & (sr_p -> sr_base);
					}
			}
			break;

		case RT_BLANK:
			{
				BlankRow *br_p = GetBlankRowFromJSON (row_json_p, plot_p, study_p, format, data_p);

				if (br_p)
					{
						row_p = & (br_p -> br_base);
					}
			}
			break;

		case RT_DISCARD:
			{
				DiscardRow *dr_p = GetDiscardRowFromJSON (row_json_p, plot_p, study_p, format, data_p);

				if (dr_p)
					{
						row_p = & (dr_p -> dr_base);
					}
			}
			break;

		case RT_GUARD:
			{
				GuardRow *gr_p = GetGuardRowFromJSON (row_json_p, plot_p, study_p, format, data_p);

				if (gr_p)
					{
						row_p = & (gr_p -> gu_base);
					}
			}
			break;

		case RT_NUM_VALUES:
			PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, row_json_p, "Row is invalid");
			break;
	}		/* switch (rt) */


	return row_p;
}


static bson_oid_t *GetNamedBSONOidFromJSON (const json_t *json_p, const char *key_s)
{
	bson_oid_t *id_p = GetNewUnitialisedBSONOid ();

	if (id_p)
		{
			if (GetNamedIdFromJSON (json_p, key_s, id_p))
				{
					return id_p;
				}		/* if (GetNamedIdFromJSON (json_p, key_s, id_p)) */
			else
				{

				}

			FreeBSONOid (id_p);
		}		/* if (id_p) */
	else
		{

		}

	return NULL;
}


void SetRowCallbackFunctions (Row *row_p,
															void (*clear_fn) (Row *row_p),
															bool (*add_to_json_fn) (const Row *row_p, json_t *row_json_p, const ViewFormat format, const FieldTrialServiceData *data_p),
															bool (*add_from_json_fn) (Row *row_p, const json_t *row_json_p, const FieldTrialServiceData *data_p),
															bool (*add_to_fd_fn) (const Row *row_p, json_t *row_fd_p, const FieldTrialServiceData *service_data_p, const char * const null_sequence_s))
{
	row_p -> ro_clear_fn = clear_fn;
	row_p -> ro_add_to_json_fn = add_to_json_fn;
	row_p -> ro_add_from_json_fn = add_from_json_fn;
	row_p -> ro_add_to_fd_fn = add_to_fd_fn;
}


/*

	bson_oid_t *ro_id_p;


};
 */


bool PopulateRowFromJSON (Row *row_p, Plot *plot_p, const json_t *row_json_p, const ViewFormat format, FieldTrialServiceData *data_p)
{
	if (!plot_p)
		{
			bson_oid_t *plot_id_p = GetNamedBSONOidFromJSON (row_json_p, RO_PLOT_ID_S);

			if (plot_id_p)
				{
					plot_p = GetPlotById (plot_id_p, NULL, format, data_p);

					if (!plot_p)
						{
							PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, row_json_p, "Failed to get plot for row");
						}

					FreeBSONOid (plot_id_p);
				}		/* if (plot_id_p) */
		}

	if (plot_p)
		{
			RowType rt = RT_STANDARD;
			bson_oid_t *id_p = GetNamedBSONOidFromJSON (row_json_p, MONGO_ID_S);

			if (id_p)
				{
					json_int_t study_index = -1;

					if (GetJSONInteger (row_json_p, RO_STUDY_INDEX_S, &study_index))
						{
							SetRowTypeFromJSON (&rt, row_json_p);

							row_p -> ro_id_p = id_p;
							row_p -> ro_type = rt;
							row_p -> ro_plot_p = plot_p;
							row_p -> ro_study_p = plot_p -> pl_parent_p;
							row_p -> ro_by_study_index = study_index;

							return true;
						}
					else
						{
							PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, row_json_p, "Failed to get study index for row using \"%s\"", RO_STUDY_INDEX_S);
						}

					FreeBSONOid (id_p);
				}		/* if (id_p) */
			else
				{
					PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, row_json_p, "Failed to get id for row using \"%s\"", MONGO_ID_S);
				}

		}		/* if (plot_p) */


	return false;
}


RowNode *AllocateRowNode (Row *row_p)
{
	RowNode *sr_node_p = (RowNode *) AllocMemory (sizeof (RowNode));

	if (sr_node_p)
		{
			InitListItem (& (sr_node_p -> rn_node));

			sr_node_p -> rn_row_p = row_p;
		}

	return sr_node_p;
}



void FreeRowNode (ListItem *node_p)
{
	RowNode *sr_node_p = (RowNode *) node_p;

	if (sr_node_p -> rn_row_p)
		{
			FreeRow (sr_node_p -> rn_row_p);
		}

	FreeMemory (sr_node_p);
}


const char *GetRowTypeAsString (const RowType rt)
{
	if (rt <= RT_NUM_VALUES)
		{
			return (* (S_ROW_TYPES_SS + rt));
		}

	return NULL;
}


bool SetRowTypeFromString (RowType *rt_p, const char *value_s)
{
	const char **type_ss = S_ROW_TYPES_SS;
	RowType r = RT_STANDARD;

	while (r < RT_NUM_VALUES)
		{
			if (strcmp (*type_ss, value_s) == 0)
				{
					*rt_p = r;
					return true;
				}
			else
				{
					++ type_ss;
					++ r;
				}
		}

	return false;
}


bool SetRowTypeFromJSON (RowType *rt_p, const json_t *row_json_p)
{
	bool success_flag = false;
	const char *type_s = GetJSONString (row_json_p, RO_ROW_TYPE_S);

	if (type_s)
		{
			success_flag = SetRowTypeFromString (rt_p, type_s);
		}		/* if (type_s) */
	else
		{
			bool b;

			if (GetJSONBoolean (row_json_p, RO_DISCARD_S, &b))
				{
					if (b)
						{
							*rt_p = RT_DISCARD;
						}
				}
			else if (GetJSONBoolean (row_json_p, RO_BLANK_S, &b))
				{
					if (b)
						{
							*rt_p = RT_BLANK;
						}
				}
		}

	return success_flag;
}

