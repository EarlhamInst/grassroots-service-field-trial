/*
 * phenotype_statistics.c
 *
 *  Created on: 27 May 2022
 *      Author: billy
 */


#include "phenotype_statistics.h"
#include "memory_allocations.h"
#include "measured_variable_jobs.h"

#include "string_utils.h"

#include "study.h"

static const char * const S_MV_ID_S = "measured_variable_id";

static json_t *GetMeasuredVariableAsJSONForStudyStorage (const MeasuredVariable *mv_p);

PhenotypeStatisticsNode *AllocatePhenotypeStatisticsNode (const char *measured_variable_name_s, const Statistics *src_p)
{
	char *mv_s = EasyCopyToNewString (measured_variable_name_s);

	if (mv_s)
		{
			bool success_flag = true;
			Statistics *copied_stats_p = NULL;

			if (src_p)
				{
					copied_stats_p = CopyStatistics (src_p);

					if (!copied_stats_p)
						{
							success_flag = false;
							PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "CopyStatistics () failed for \"%s\"", measured_variable_name_s);
						}
				}

			if (success_flag)
				{
					PhenotypeStatisticsNode *node_p = (PhenotypeStatisticsNode *) AllocMemory (sizeof (PhenotypeStatisticsNode));

					if (node_p)
						{
							InitListItem (& (node_p -> psn_node));
							node_p -> psn_measured_variable_name_s = mv_s;
							node_p -> psn_stats_p = copied_stats_p;

							return node_p;
						}
					else
						{
							PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to allocate stats node for measured variable name \"%s\"", measured_variable_name_s);
						}

					if (copied_stats_p)
						{
							FreeStatistics (copied_stats_p);
						}
				}
			else
				{
					PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to copy statistics for measured variable name \"%s\"", measured_variable_name_s);
				}

			FreeCopiedString (mv_s);
		}
	else
		{
			PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to copy measured variable name \"%s\"", measured_variable_name_s);
		}

	return NULL;
}


void FreePhenotypeStatisticsNode (ListItem *node_p)
{
	PhenotypeStatisticsNode *psn_p = (PhenotypeStatisticsNode *) node_p;

	FreeCopiedString (psn_p -> psn_measured_variable_name_s);

	if (psn_p -> psn_stats_p)
		{
			FreeStatistics (psn_p -> psn_stats_p);
		}

	FreeMemory (psn_p);
}


bool AddPhenotypeStatisticsNodeAsJSON (const PhenotypeStatisticsNode *psn_p, json_t *parent_p, const ViewFormat format, const FieldTrialServiceData *service_data_p)
{
	bool success_flag = false;
	MEM_FLAG mv_mf;
	MeasuredVariable *mv_p = GetMeasuredVariableByVariableName (psn_p -> psn_measured_variable_name_s, &mv_mf, service_data_p);

	if (mv_p)
		{
			json_t *phenotype_p = json_object ();

			if (phenotype_p)
				{
					json_t *mv_json_p = NULL;

					if (format == VF_STORAGE)
						{
							mv_json_p = GetMeasuredVariableAsJSONForStudyStorage (mv_p);
						}
					else
						{
							mv_json_p = GetMeasuredVariableAsJSON (mv_p, format, service_data_p);
						}

					if (mv_json_p)
						{
							if (json_object_set_new (phenotype_p, ST_PHENOTYPE_DEFINITION_S, mv_json_p) == 0)
								{
									/*
									 * We don't need the stats if we're indexing the study
									 */
									if (format != VF_INDEXING)
										{
											json_t *stats_json_p = NULL;

											if (psn_p -> psn_stats_p)
												{
													stats_json_p = GetStatisticsAsJSON (psn_p -> psn_stats_p);
												}
											else
												{
													stats_json_p = json_null ();
												}

											if (stats_json_p)
												{
													if (json_object_set_new (phenotype_p, ST_PHENOTYPE_STATISTICS_S, stats_json_p) == 0)
														{
															success_flag = true;
														}
													else
														{
															PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, stats_json_p, "Failed to add stats json to phenotype json");
															json_decref (stats_json_p);
														}
												}
											else
												{
													PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to gets stats as json for \"%s\"", psn_p -> psn_measured_variable_name_s);
												}


										}		/* if (format != VF_INDEXING) */
									else
										{
											success_flag = true;
										}

									if (success_flag)
										{
											int res = -1;
											/*
											 * On the front ends have the phenotypes as objects to allow for quick lookup
											 * On the server , to take advantage of using an index for fast searching, the
											 * phenotypes need to be in an array
											 */
											if (format == VF_STORAGE)
												{

													res = json_array_append_new (parent_p, phenotype_p);
												}
											else
												{
													res = json_object_set_new (parent_p, psn_p -> psn_measured_variable_name_s, phenotype_p);
												}


											if (res == 0)
												{
													success_flag = true;
												}
											else
												{
													PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, phenotype_p, "Failed to add phenotype json for \"%s\" to parent json", psn_p -> psn_measured_variable_name_s);
												}

										}

								}
							else
								{
									PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, mv_json_p, "Failed to add Measured Variable json to phenotype json");
									json_decref (mv_json_p);
								}
						}
					else
						{
							PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to get json for MeasuredVariable \"%s\"", psn_p -> psn_measured_variable_name_s);
						}

					if (!success_flag)
						{
							json_decref (phenotype_p);
						}

				}		/* if (phenotype_p) */


			/*
			 * If we own the MeasuredVariable, free it
			 */
			if ((mv_mf == MF_DEEP_COPY) || (mv_mf == MF_SHALLOW_COPY))
				{
					FreeMeasuredVariable (mv_p);
				}

		}		/* if (mv_p) */
	else
		{
			PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to get MeasuredVariable \"%s\"", psn_p -> psn_measured_variable_name_s);
		}

	return success_flag;
}


bool AddPhenotypeStatisticsNodeFromJSON (Study *study_p, const json_t *phenotype_p, const FieldTrialServiceData *service_data_p)
{
	bool success_flag = false;
	const json_t *mv_json_p = json_object_get (phenotype_p, ST_PHENOTYPE_DEFINITION_S);

	if (mv_json_p)
		{
			MeasuredVariable *mv_p = NULL;
			bson_oid_t *mv_id_p = GetNewUnitialisedBSONOid ();

			if (mv_id_p)
				{
					if (GetNamedIdFromJSON (mv_json_p, S_MV_ID_S, mv_id_p))
						{
							mv_p = GetMeasuredVariableById (mv_id_p, service_data_p);

							if (!mv_p)
								{
									PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, mv_json_p, "GetMeasuredVariableById () failed");
								}
						}
					else
						{
							PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, mv_json_p, "GetNamedIdFromJSON () for \"%s\" failed", S_MV_ID_S);
						}

					FreeBSONOid (mv_id_p);
				}
			else
				{
					PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, mv_json_p, "GetNewUnitialisedBSONOid () failed");
				}


			if (mv_p)
				{
					Statistics *stats_p = NULL;
					const json_t *stats_json_p = json_object_get (phenotype_p, ST_PHENOTYPE_STATISTICS_S);
					bool stats_flag = true;

					if ((stats_json_p) && (stats_json_p != json_null ()))
						{
							stats_p = GetStatisticsFromJSON (stats_json_p);

							if (!stats_p)
								{
									stats_flag = false;
								}
						}

					if (stats_flag)
						{
							const char *mv_s = GetMeasuredVariableName (mv_p);

							if (AddPhenotypeStatisticsToStudy (study_p, mv_s, stats_p))
								{
									success_flag = true;
								}

						}

					if (stats_p)
						{
							FreeStatistics (stats_p);
						}

					FreeMeasuredVariable (mv_p);
				}		/* if (mv_p) */

		}		/* if (mv_json_p) */

	return success_flag;
}



static json_t *GetMeasuredVariableAsJSONForStudyStorage (const MeasuredVariable *mv_p)
{
	json_t *mv_json_p = json_object ();

	if (mv_json_p)
		{
			if (SetJSONString (mv_json_p, MV_NAME_S, mv_p -> mv_variable_term_p -> st_name_s))
				{
					if (AddNamedCompoundIdToJSON (mv_json_p, mv_p -> mv_id_p, S_MV_ID_S))
						{
							return mv_json_p;
						}		/* if (AddCompoundIdToJSON (mv_json_p, mv_p -> mv_id_p)) */
					else
						{
							PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, mv_json_p, "Failed to add \"%s\": \"%s\"", MV_NAME_S, mv_p -> mv_variable_term_p -> st_name_s);
						}
				}		/* if (SetJSONString (mv_json_p, MV_NAME_S, mv_p -> mv_variable_term_p -> st_name_s)) */
			else
				{
					PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, mv_json_p, "Failed to add \"%s\": \"%s\"", MV_NAME_S, mv_p -> mv_variable_term_p -> st_name_s);
				}

			json_decref (mv_json_p);
		}		/* if (mv_json_p) */
	else
		{
			PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, mv_json_p, "Failed to allocate json for \"%s\"", mv_p -> mv_variable_term_p -> st_name_s);
		}

	return NULL;
}
