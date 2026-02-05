/*
 ** Copyright 2014-2018 The Earlham Institute
 **
 ** Licensed under the Apache License, Version 2.0 (the "License");
 ** you may not use this file except in compliance with the License.
 ** You may obtain a copy of the License at
 **
 **     http://www.apache.org/licenses/LICENSE-2.0
 **
 ** Unless required by applicable law or agreed to in writing, software
 ** distributed under the License is distributed on an "AS IS" BASIS,
 ** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 ** See the License for the specific language governing permissions and
 ** limitations under the License.
 */
/*
 * indexing.c
 *
 *  Created on: 23 Aug 2019
 *      Author: billy
 */

#include <stddef.h>
#include <string.h>
#include <time.h>


#include "indexing.h"
#include "study_jobs.h"
#include "location_jobs.h"
#include "field_trial_jobs.h"
#include "measured_variable_jobs.h"
#include "programme_jobs.h"
#include "treatment_jobs.h"
#include "audit.h"
#include "row_jobs.h"


#include "boolean_parameter.h"
#include "json_parameter.h"
#include "string_utils.h"
#include "math_utils.h"
#include "time_util.h"

#include "lucene_tool.h"
#include "grassroots_server.h"
#include "providers_state_table.h"
#include "schema_term.h"
#include "service.h"
#include "service_job.h"
#include "service_metadata.h"
#include "handler.h"
#include "parameter.h"
#include "parameter_group.h"
#include "parameter_set.h"
#include "parameter_type.h"
#include "string_parameter.h"
#include "linked_list.h"
#include "string_linked_list.h"
#include "data_resource.h"
#include "filesystem_utils.h"
#include "streams.h"
#include "json_util.h"
#include "memory_allocations.h"
#include "operation.h"
#include "schema_keys.h"
#include "typedefs.h"
#include "user_details.h"
#include "dfw_field_trial_service_data.h"
#include "handbook_generator.h"
#include "crop_ontology_tool.h"

#include <bson/bson.h>
#include "mongodb_tool.h"
#include "stdbool.h"
#include "study.h"
#include "jansson.h"

#include "material.h"
#include "plot.h"
#include "measured_variable.h"

/*
 * Static declarations
 */

/*
 * indexing parameters
 */
static NamedParameterType S_CLEAR_DATA = { "SS Clear all data", PT_BOOLEAN };
static NamedParameterType S_REINDEX_ALL_DATA = { "SS Reindex all data", PT_BOOLEAN };
static NamedParameterType S_REINDEX_TRIALS = { "SS Reindex trials", PT_BOOLEAN };
static NamedParameterType S_REINDEX_STUDIES = { "SS Reindex studies", PT_BOOLEAN };
static NamedParameterType S_REINDEX_LOCATIONS = { "SS Reindex locations", PT_BOOLEAN };
static NamedParameterType S_REINDEX_MEASURED_VARIABLES = { "SS Reindex measured variables", PT_BOOLEAN };
static NamedParameterType S_REINDEX_PROGRAMS = { "SS Reindex programs", PT_BOOLEAN };
static NamedParameterType S_REINDEX_TREATMENTS = { "SS Reindex treatments", PT_BOOLEAN };


/*
 * caching parameters
 */
static NamedParameterType S_CACHE_CLEAR = { "SS clear study cache", PT_LARGE_STRING };
static NamedParameterType S_CACHE_LIST = { "SS list study cache", PT_BOOLEAN };


/*
 * study management parameters
 */
static NamedParameterType S_REMOVE_STUDIES = { "SS Delete studies", PT_LARGE_STRING };
static NamedParameterType S_GENERATE_FD_PACKAGES = { "SS Generate FD Packages", PT_LARGE_STRING};
static NamedParameterType S_REMOVE_STUDY_PLOTS = { "SS Remove Study Plots", PT_STRING };
static NamedParameterType S_GENERATE_HANDBOOK = { "SS Generate Handbook", PT_STRING };
static NamedParameterType S_UPDATE_SCALE_TERMS = { "SS Update Scale Classes", PT_BOOLEAN };

static NamedParameterType S_ROTHAMSTED_TERMS = { "SS Roth Upload", PT_JSON_TABLE };

static NamedParameterType S_GENERATE_STUDY_METADATA = { "SS Generate Phenotypes & Accessions", PT_LARGE_STRING };


static NamedParameterType S_ADD_MONGODB_INDEXES = { "SS Add MongoDB Indexes", PT_BOOLEAN };


static const char *GetFieldTrialIndexingServiceName (const Service *service_p);

static const char *GetFieldTrialIndexingServiceDescription (const Service *service_p);

static const char *GetFieldTrialIndexingServiceAlias (const Service *service_p);

static const char *GetFieldTrialIndexingServiceInformationUri (const Service *service_p);


static bool GetIndexingParameterTypeForNamedParameter (const Service *service_p, const char *param_name_s, ParameterType *pt_p);

static ParameterSet *GetFieldTrialIndexingServiceParameters (Service *service_p, DataResource *resource_p, User *user_p);

static ServiceJobSet *RunFieldTrialIndexingService (Service *service_p, ParameterSet *param_set_p, User *user_p, ProvidersStateTable *providers_p);


static bool RunReindexing (ParameterSet *param_set_p, ServiceJob *job_p, FieldTrialServiceData *data_p);

static bool RunCaching (ParameterSet *param_set_p, ServiceJob *job_p, FieldTrialServiceData *data_p);


static ParameterSet *IsResourceForFieldTrialIndexingService (Service *service_p, DataResource *resource_p, Handler *handler_p);

static ServiceMetadata *GetFieldTrialIndexingServiceMetadata (Service *service_p);


static void ReleaseFieldTrialIndexingServiceParameters (Service *service_p, ParameterSet *params_p);


static bool CloseFieldTrialIndexingService (Service *service_p);

static void GetCacheList (ServiceJob *job_p, const bool full_path_flag, const FieldTrialServiceData *data_p);

static LinkedList *GetFieldTrialFiles (const char * const path_s, const char * const local_pattern_s, const bool full_path_flag);

static OperationStatus GenerateAllFrictionlessDataStudies (ServiceJob *job_p, FieldTrialServiceData *data_p);


static OperationStatus StoreAllRResScaleUnits (json_t *terms_p, FieldTrialServiceData *data_p);

static OperationStatus CreateMongoIndexes (FieldTrialServiceData *data_p);

static void GenerateStudyHandbook (Study *study_p, ServiceJob *job_p, FieldTrialServiceData *data_p);


static OperationStatus CreateMongoRevisionsCollections (FieldTrialServiceData *data_p);

static OperationStatus CreateMongoRevisionsCollection (MongoTool *tool_p, const char *database_s, const char *collection_s);


/*
 * API definitions
 */


Service *GetFieldTrialIndexingService (GrassrootsServer *grassroots_p)
{
	Service *service_p = (Service *) AllocMemory (sizeof (Service));

	if (service_p)
		{
			FieldTrialServiceData *data_p = AllocateFieldTrialServiceData ();

			if (data_p)
				{
					if (InitialiseService (service_p,
																 GetFieldTrialIndexingServiceName,
																 GetFieldTrialIndexingServiceDescription,
																 GetFieldTrialIndexingServiceAlias,
																 GetFieldTrialIndexingServiceInformationUri,
																 RunFieldTrialIndexingService,
																 IsResourceForFieldTrialIndexingService,
																 GetFieldTrialIndexingServiceParameters,
																 GetIndexingParameterTypeForNamedParameter,
																 ReleaseFieldTrialIndexingServiceParameters,
																 CloseFieldTrialIndexingService,
																 NULL,
																 false,
																 SY_SYNCHRONOUS,
																 (ServiceData *) data_p,
																 GetFieldTrialIndexingServiceMetadata,
																 NULL,
																 grassroots_p))
						{

							if (ConfigureFieldTrialService (data_p, grassroots_p))
								{
									return service_p;
								}

						}		/* if (InitialiseService (.... */

					FreeFieldTrialServiceData (data_p);
				}

			FreeMemory (service_p);
		}		/* if (service_p) */

	return NULL;
}

/*
 * Static definitions
 */

static const char *GetFieldTrialIndexingServiceName (const Service * UNUSED_PARAM (service_p))
{
	return "Manage Field Trial data";
}


static const char *GetFieldTrialIndexingServiceDescription (const Service * UNUSED_PARAM (service_p))
{
	return "A service to manage field trial data";
}


static const char *GetFieldTrialIndexingServiceAlias (const Service * UNUSED_PARAM (service_p))
{
	return DFT_GROUP_ALIAS_PREFIX_S SERVICE_GROUP_ALIAS_SEPARATOR "manager";
}


static const char *GetFieldTrialIndexingServiceInformationUri (const Service * UNUSED_PARAM (service_p))
{
	return NULL;
}


static bool CloseFieldTrialIndexingService (Service *service_p)
{
	bool success_flag = true;

	FreeFieldTrialServiceData ((FieldTrialServiceData *) (service_p -> se_data_p));

	return success_flag;
}



static bool RunReindexing (ParameterSet *param_set_p, ServiceJob *job_p, FieldTrialServiceData *data_p)
{
	bool done_flag = false;
	OperationStatus status = GetServiceJobStatus (job_p);
	const bool *index_flag_p = NULL;
	const bool *clear_flag_p = NULL;
	bool update_flag = false;

	if (GetCurrentBooleanParameterValueFromParameterSet (param_set_p, S_CLEAR_DATA.npt_name_s, &clear_flag_p))
		{
			if (clear_flag_p)
				{
					update_flag = ! (*clear_flag_p);
				}
		}

	if (GetCurrentBooleanParameterValueFromParameterSet (param_set_p, S_REINDEX_ALL_DATA.npt_name_s, &index_flag_p))
		{
			if ((index_flag_p != NULL) && (*index_flag_p == true))
				{
					ReindexAllData (job_p, update_flag, data_p);

					done_flag = true;
				}
		}

	if (!done_flag)
		{
			GrassrootsServer *grassroots_p = GetGrassrootsServerFromService (job_p -> sj_service_p);
			LuceneTool *lucene_p = AllocateLuceneTool (grassroots_p, job_p -> sj_id);
			uint32 num_attempted = 0;
			uint32 num_succeeded = 0;

			if (lucene_p)
				{
					if (GetCurrentBooleanParameterValueFromParameterSet (param_set_p, S_REINDEX_TRIALS.npt_name_s, &index_flag_p))
						{
							if ((index_flag_p != NULL) && (*index_flag_p == true))
								{
									if (ReindexTrials (job_p, lucene_p, update_flag, data_p))
										{
											++ num_succeeded;
										}

									++ num_attempted;

									update_flag = true;
									done_flag = true;
								}
						}

					if (GetCurrentBooleanParameterValueFromParameterSet (param_set_p, S_REINDEX_STUDIES.npt_name_s, &index_flag_p))
						{
							if ((index_flag_p != NULL) && (*index_flag_p == true))
								{
									if (ReindexStudies (job_p, lucene_p, update_flag, data_p))
										{
											++ num_succeeded;
										}

									++ num_attempted;


									update_flag = true;
									done_flag = true;
								}
						}

					if (GetCurrentBooleanParameterValueFromParameterSet (param_set_p, S_REINDEX_LOCATIONS.npt_name_s, &index_flag_p))
						{
							if ((index_flag_p != NULL) && (*index_flag_p == true))
								{
									if (ReindexLocations (job_p, lucene_p, update_flag, data_p))
										{
											++ num_succeeded;
										}

									++ num_attempted;

									update_flag = true;
									done_flag = true;
								}
						}

					if (GetCurrentBooleanParameterValueFromParameterSet (param_set_p, S_REINDEX_MEASURED_VARIABLES.npt_name_s, &index_flag_p))
						{
							if ((index_flag_p != NULL) && (*index_flag_p == true))
								{
									if (ReindexMeasuredVariables (job_p, lucene_p, update_flag, data_p))
										{
											++ num_succeeded;
										}

									++ num_attempted;

									update_flag = true;
									done_flag = true;
								}
						}

					if (GetCurrentBooleanParameterValueFromParameterSet (param_set_p, S_REINDEX_PROGRAMS.npt_name_s, &index_flag_p))
						{
							if ((index_flag_p != NULL) && (*index_flag_p == true))
								{
									if (ReindexProgrammes (job_p, lucene_p, update_flag, data_p))
										{
											++ num_succeeded;
										}

									++ num_attempted;

									update_flag = true;
									done_flag = true;
								}
						}


					if (GetCurrentBooleanParameterValueFromParameterSet (param_set_p, S_REINDEX_TREATMENTS.npt_name_s, &index_flag_p))
						{
							if ((index_flag_p != NULL) && (*index_flag_p == true))
								{
									if (ReindexTreatments (job_p, lucene_p, update_flag, data_p))
										{
											++ num_succeeded;
										}

									++ num_attempted;

									update_flag = true;
									done_flag = true;
								}
						}

					FreeLuceneTool (lucene_p);
				}		/* if (lucene_p) */

			if (num_succeeded == num_attempted)
				{
					status = OS_SUCCEEDED;
				}
			else if (num_succeeded > 0)
				{
					status = OS_PARTIALLY_SUCCEEDED;
				}
			else
				{
					status = OS_FAILED;
				}

			SetServiceJobStatus (job_p, status);

		}		/* if (!done_flag) */


	return done_flag;
}


static bool RunCaching (ParameterSet *param_set_p, ServiceJob *job_p, FieldTrialServiceData *data_p)
{
	bool done_flag = false;
	OperationStatus status = GetServiceJobStatus (job_p);
	const bool *index_flag_p = NULL;

	if (data_p -> dftsd_study_cache_path_s)
		{
			if (GetCurrentBooleanParameterValueFromParameterSet (param_set_p, S_CACHE_LIST.npt_name_s, &index_flag_p))
				{
					if ((index_flag_p != NULL) && (*index_flag_p == true))
						{
							GetCacheList (job_p, false, data_p);

							done_flag = true;
						}
				}

			if (!done_flag)
				{
					const char *entries_s = NULL;

					if (GetCurrentStringParameterValueFromParameterSet (param_set_p, S_CACHE_CLEAR.npt_name_s, &entries_s))
						{
							if (!IsStringEmpty (entries_s))
								{
									LinkedList *entries_p = NULL;

									if (strcmp (entries_s, "*") == 0)
										{
											entries_p = GetFieldTrialFiles (data_p -> dftsd_study_cache_path_s, "*.json", false);
										}
									else
										{
											/*
											 * Get the cache entries to clear
											 */
											entries_p = ParseStringToStringLinkedList (entries_s, " ", true);
										}


									if (entries_p)
										{
											StringListNode *node_p = (StringListNode *) (entries_p -> ll_head_p);
											size_t num_removed = 0;

											while (node_p)
												{
													char *filename_s = GetFullCacheFilename (node_p -> sln_string_s, data_p);

													if (filename_s)
														{
															if (RemoveFile (filename_s))
																{
																	++ num_removed;
																}
															else
																{
																	PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to remove file \"%s\"", node_p -> sln_string_s);
																}

															if (filename_s != node_p -> sln_string_s)
																{
																	FreeCopiedString (filename_s);
																}
														}


													node_p = (StringListNode *) (node_p -> sln_node.ln_next_p);
												}

											if (num_removed == entries_p -> ll_size)
												{
													status = OS_SUCCEEDED;
												}
											else if (num_removed > 0)
												{
													status = OS_PARTIALLY_SUCCEEDED;
												}


											FreeLinkedList (entries_p);
										}
								}

						}

					SetServiceJobStatus (job_p, status);
				}		/* if (!done_flag) */

		}		/* if (data_p -> dftsd_study_cache_path_s) */
	else
		{
			PrintLog (STM_LEVEL_INFO, __FILE__, __LINE__, "No cache path has been set");
		}

	return done_flag;
}


static void GetCacheList (ServiceJob *job_p, const bool full_path_flag, const FieldTrialServiceData *data_p)
{
	if (data_p -> dftsd_study_cache_path_s)
		{
			LinkedList *filenames_p = GetFieldTrialFiles (data_p -> dftsd_study_cache_path_s, "*.json", true);

			if (filenames_p)
				{
					OperationStatus status = OS_FAILED;
					json_t *files_array_p = json_array ();

					if (files_array_p)
						{
							size_t array_size;
							StringListNode *node_p = (StringListNode *) (filenames_p -> ll_head_p);
							FileInformation info;
							json_t *dest_record_p = NULL;
							const char sep = GetFileSeparatorChar ();

							InitFileInformation (&info);

							while (node_p)
								{
									if (CalculateFileInformation (node_p -> sln_string_s, &info))
										{
											const char *filename_s;

											if (full_path_flag)
												{
													filename_s = node_p -> sln_string_s;
												}
											else
												{
													filename_s = strrchr (node_p -> sln_string_s, sep);

													if (filename_s)
														{
															/* scroll past the file separator char */
															++ filename_s;
														}
												}


											if (filename_s)
												{
													json_t *file_p = json_object ();

													if (file_p)
														{
															bool added_flag = false;

															if (SetJSONString (file_p, CONTEXT_PREFIX_SCHEMA_ORG_S "name", filename_s))
																{
																	struct tm *timestamp_p = localtime (& (info.fi_last_modified));

																	if (timestamp_p)
																		{
																			char *time_s = GetTimeAsString (timestamp_p, true, NULL);

																			if (time_s)
																				{
																					if (SetJSONString (file_p, CONTEXT_PREFIX_SCHEMA_ORG_S "DateTime", time_s))
																						{
																							char *bytes_s = ConvertLongToString (info.fi_size);

																							if (bytes_s)
																								{
																									char *size_s = ConcatenateStrings (bytes_s, " B");

																									if (size_s)
																										{
																											if (SetJSONString (file_p, CONTEXT_PREFIX_SCHEMA_ORG_S "fileSize", size_s))
																												{
																													if (json_array_append_new (files_array_p, file_p) == 0)
																														{
																															added_flag = true;
																														}		/* if (json_array_append_new (files_array_p, file_p) == 0) */
																													else
																														{
																															PrintJSONToErrors (STM_LEVEL_INFO, __FILE__, __LINE__, file_p, "Failed to add json item for \"%s\"", node_p -> sln_string_s);
																														}

																												}		/* if (SetJSONString (file_p, CONTEXT_PREFIX_SCHEMA_ORG_S "fileSize", size_s)) */
																											else
																												{
																													PrintJSONToErrors (STM_LEVEL_INFO, __FILE__, __LINE__, file_p, "Failed to set \"%s\": \"%s\" for \"%s\"", CONTEXT_PREFIX_SCHEMA_ORG_S "fileSize", size_s, node_p -> sln_string_s);
																												}

																											FreeCopiedString (size_s);
																										}		/* if (size_s) */
																									else
																										{
																											PrintErrors (STM_LEVEL_INFO, __FILE__, __LINE__, "ConcatenateStrings failed for \"%s\" and \" B\" for \"%s\"", bytes_s, node_p -> sln_string_s);
																										}

																									FreeCopiedString (bytes_s);
																								}		/* if (bytes_s) */
																							else
																								{
																									PrintErrors (STM_LEVEL_INFO, __FILE__, __LINE__, "Failed to get size as string from " SIZET_FMT " for  \"%s\"", info.fi_size, node_p -> sln_string_s);
																								}

																						}		/* if (SetJSONString (file_p, CONTEXT_PREFIX_SCHEMA_ORG_S "DateTime", time_s)) */
																					else
																						{
																							PrintJSONToErrors (STM_LEVEL_INFO, __FILE__, __LINE__, file_p, "Failed to set \"%s\": \"%s\" for \"%s\"", CONTEXT_PREFIX_SCHEMA_ORG_S "DateTime", time_s, node_p -> sln_string_s);
																						}

																					FreeCopiedString (time_s);
																				}		/* if (time_s) */
																			else
																				{
																					PrintJSONToErrors (STM_LEVEL_INFO, __FILE__, __LINE__, file_p, "GetTimeAsString for \"%s\"", node_p -> sln_string_s);
																				}

																		}		/* if (localtime_r (& (info.fi_last_modified), &timestamp)) */
																	else
																		{
																			PrintJSONToErrors (STM_LEVEL_INFO, __FILE__, __LINE__, file_p, "Failed to calculate local time for \"%s\"", node_p -> sln_string_s);
																		}

																}		/* if (SetJSONString (file_p, CONTEXT_PREFIX_SCHEMA_ORG_S "name", filename_s)) */
															else
																{
																	PrintJSONToErrors (STM_LEVEL_INFO, __FILE__, __LINE__, file_p, "Failed to set \"%s\": \"%s\" for \"%s\"", CONTEXT_PREFIX_SCHEMA_ORG_S "name", filename_s, node_p -> sln_string_s);
																}


															if (!added_flag)
																{
																	json_decref (file_p);
																}

														}		/* if (file_p) */
													else
														{
															PrintErrors (STM_LEVEL_INFO, __FILE__, __LINE__, "Failed to allocate JSON cache data for \"%s\"", node_p -> sln_string_s);
														}

												}		/* if (filename_s) */
											else
												{
													PrintErrors (STM_LEVEL_INFO, __FILE__, __LINE__, "Could not determine local filename from \"%s\"", node_p -> sln_string_s);
												}

										}		/* if (CalculateFileInformation (node_p -> sln_string_s, &info)) */
									else
										{
											PrintErrors (STM_LEVEL_INFO, __FILE__, __LINE__, "CalculateFileInformation failed for in \"%s\"", node_p -> sln_string_s);
										}

									node_p = (StringListNode *) (node_p -> sln_node.ln_next_p);
								}		/* while (node_p) */

							array_size = json_array_size (files_array_p);

							if (filenames_p -> ll_size == array_size)
								{
									status = OS_SUCCEEDED;
								}
							else if (array_size > 0)
								{
									status = OS_PARTIALLY_SUCCEEDED;
								}

							if (status != OS_FAILED)
								{
									dest_record_p = GetDataResourceAsJSONByParts (PROTOCOL_INLINE_S, NULL, "Cached Files", files_array_p);

									if (dest_record_p)
										{
											if (!AddResultToServiceJob (job_p, dest_record_p))
												{
													json_decref (dest_record_p);
													status = OS_FAILED;
													PrintErrors (STM_LEVEL_INFO, __FILE__, __LINE__, "AddResultToServiceJob failed for cached files in \"%s\"", data_p -> dftsd_study_cache_path_s);
												}

										}		/* if (dest_record_p) */
									else
										{
											status = OS_FAILED;
											PrintErrors (STM_LEVEL_INFO, __FILE__, __LINE__, "GetResourceAsJSONByParts failed for cached files in \"%s\"", data_p -> dftsd_study_cache_path_s);
										}
								}		/* if (status != OS_FAILED */
							else
								{
									PrintErrors (STM_LEVEL_INFO, __FILE__, __LINE__, "Failed getting cached files data in \"%s\"", data_p -> dftsd_study_cache_path_s);
								}

						}

					SetServiceJobStatus (job_p, status);
					FreeLinkedList (filenames_p);
				}		/* if (filenames_p) */
			else
				{
					PrintErrors (STM_LEVEL_INFO, __FILE__, __LINE__, "No cached files in \"%s\"", data_p -> dftsd_study_cache_path_s);
				}

		}		/* if (data_p -> dftsd_study_cache_path_s) */
	else
		{
			PrintErrors (STM_LEVEL_INFO, __FILE__, __LINE__, "No studies cache path set");
		}

}


static LinkedList *GetFieldTrialFiles (const char * const path_s, const char * const local_pattern_s, const bool full_path_flag)
{
	LinkedList *files_p = NULL;
	char *pattern_s = NULL;

	if (local_pattern_s)
		{
			pattern_s = ConcatenateStrings (path_s, local_pattern_s);
		}
	else
		{
			pattern_s = (char *) path_s;
		}

	if (pattern_s)
		{
			if (! (files_p = GetMatchingFiles (pattern_s, full_path_flag)))
				{
					PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "GetMatchingFiles failed for pattern: \"%s\"", pattern_s);
				}

			if (pattern_s != path_s)
				{
					FreeCopiedString (pattern_s);
				}
		}		/* if (pattern_s) */
	else
		{
			PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to concat strings for  path \"%s\" and file pattern: \"%s\"", path_s, local_pattern_s);
		}

	return files_p;
}




OperationStatus ReindexAllData (ServiceJob *job_p, const bool update_flag, const FieldTrialServiceData *service_data_p)
{
	OperationStatus status = OS_FAILED_TO_START;
	GrassrootsServer *grassroots_p = GetGrassrootsServerFromService (job_p -> sj_service_p);
	LuceneTool *lucene_p = AllocateLuceneTool (grassroots_p, job_p -> sj_id);

	if (lucene_p)
		{
			OperationStatus temp_status = ReindexStudies (job_p, lucene_p, update_flag, service_data_p);
			uint32 fully_succeeded_count = 0;
			uint32 partially_succeeded_count = 0;
			uint32 total_count = 0;

			++ total_count;
			if (temp_status == OS_SUCCEEDED)
				{
					++ fully_succeeded_count;
				}
			else if (temp_status == OS_PARTIALLY_SUCCEEDED)
				{
					++ partially_succeeded_count;
				}


			temp_status = ReindexTrials (job_p, lucene_p, true, service_data_p);
			++ total_count;

			if (temp_status == OS_SUCCEEDED)
				{
					++ fully_succeeded_count;
				}
			else if (temp_status == OS_PARTIALLY_SUCCEEDED)
				{
					++ partially_succeeded_count;
				}

			temp_status = ReindexLocations (job_p, lucene_p, true, service_data_p);
			++ total_count;

			if (temp_status == OS_SUCCEEDED)
				{
					++ fully_succeeded_count;
				}
			else if (temp_status == OS_PARTIALLY_SUCCEEDED)
				{
					++ partially_succeeded_count;
				}

			temp_status = ReindexMeasuredVariables (job_p, lucene_p, true, service_data_p);
			++ total_count;

			if (temp_status == OS_SUCCEEDED)
				{
					++ fully_succeeded_count;
				}
			else if (temp_status == OS_PARTIALLY_SUCCEEDED)
				{
					++ partially_succeeded_count;
				}

			temp_status = ReindexProgrammes (job_p, lucene_p, true, service_data_p);
			++ total_count;
			if (temp_status == OS_SUCCEEDED)
				{
					++ fully_succeeded_count;
				}
			else if (temp_status == OS_PARTIALLY_SUCCEEDED)
				{
					++ partially_succeeded_count;
				}


			temp_status = ReindexTreatments (job_p, lucene_p, true, service_data_p);
			++ total_count;
			if (temp_status == OS_SUCCEEDED)
				{
					++ fully_succeeded_count;
				}
			else if (temp_status == OS_PARTIALLY_SUCCEEDED)
				{
					++ partially_succeeded_count;
				}


			if (fully_succeeded_count == total_count)
				{
					status = OS_SUCCEEDED;
				}
			else if ((fully_succeeded_count > 0) || (partially_succeeded_count > 0))
				{
					status = OS_PARTIALLY_SUCCEEDED;
				}


			FreeLuceneTool (lucene_p);
		}		/* if (lucene_p) */

	SetServiceJobStatus (job_p, status);

	return status;
}


OperationStatus ReindexStudies (ServiceJob *job_p, LuceneTool *lucene_p, bool update_flag, const FieldTrialServiceData *service_data_p)
{
	OperationStatus status = OS_FAILED;
	json_t *id_results_p = GetAllStudyIds (service_data_p -> dftsd_base_data.sd_service_p);


	if (id_results_p)
		{
			json_t *id_result_p;
			size_t i;
			size_t num_successes = 0;

			json_array_foreach (id_results_p, i, id_result_p)
				{
					bson_oid_t id;

					if (GetMongoIdFromJSON (id_result_p, &id))
						{
							char *id_s = GetBSONOidAsString (&id);

							if (id_s)
								{
									Study *study_p = GetStudyByIdString (id_s, VF_CLIENT_FULL, service_data_p);

									if (study_p)
										{
											OperationStatus s = IndexStudy (study_p, job_p, id_s, service_data_p);

											FreeStudy (study_p);
										}		/* if (study_p) */
									else
										{
											PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "GetStudyByIdString () failed for \"%s\"", id_s);
										}
								}		/* if (id_s) */
							else
								{
									PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, id_result_p, "GetBSONOidAsString () failed");
								}

						}		/* if (GetMongoIdFromJSON (id_result_p, &id)) */
					else
						{
							PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, id_result_p, "GetMongoIdFromJSON () failed");
						}

				}		/* json_array_foreach (id_results_p, i, id_result_p) */

			json_decref (id_results_p);
		}		/* if (id_results_p) */


	return status;
}



OperationStatus ReindexTreatments (ServiceJob *job_p, LuceneTool *lucene_p, bool update_flag, const FieldTrialServiceData *service_data_p)
{
	OperationStatus status = OS_FAILED;
	json_t *treatments_p = GetTreatmentIndexingData (service_data_p -> dftsd_base_data.sd_service_p);

	if (treatments_p)
		{
			if (SetLuceneToolName (lucene_p, "index_treatments"))
				{
					status = IndexLucene (lucene_p, treatments_p, update_flag);
				}

			json_decref (treatments_p);
		}

	return status;
}


OperationStatus ReindexProgrammes (ServiceJob *job_p, LuceneTool *lucene_p, bool update_flag, const FieldTrialServiceData *service_data_p)
{
	OperationStatus status = OS_FAILED;
	json_t *programmes_p = GetProgrammeIndexingData (service_data_p -> dftsd_base_data.sd_service_p);

	if (programmes_p)
		{
			if (SetLuceneToolName (lucene_p, "index_programmes"))
				{
					status = IndexLucene (lucene_p, programmes_p, update_flag);
				}

			json_decref (programmes_p);
		}

	return status;
}


OperationStatus ReindexLocations (ServiceJob *job_p, LuceneTool *lucene_p, bool update_flag, const FieldTrialServiceData *service_data_p)
{
	OperationStatus status = OS_FAILED;
	json_t *locations_p = GetLocationIndexingData (service_data_p -> dftsd_base_data.sd_service_p);

	if (locations_p)
		{
			if (SetLuceneToolName (lucene_p, "index_locations"))
				{
					status = IndexLucene (lucene_p, locations_p, update_flag);
				}
			json_decref (locations_p);
		}

	return status;
}


OperationStatus ReindexTrials (ServiceJob *job_p, LuceneTool *lucene_p, bool update_flag, const FieldTrialServiceData *service_data_p)
{
	OperationStatus status = OS_FAILED;
	json_t *trials_p = GetFieldTrialIndexingData (service_data_p -> dftsd_base_data.sd_service_p);

	if (trials_p)
		{
			if (SetLuceneToolName (lucene_p, "index_trials"))
				{
					status = IndexLucene (lucene_p, trials_p, update_flag);
				}

			json_decref (trials_p);
		}

	return status;
}


OperationStatus ReindexMeasuredVariables (ServiceJob *job_p, LuceneTool *lucene_p, bool update_flag, const FieldTrialServiceData *service_data_p)
{
	OperationStatus status = OS_FAILED;
	json_t *id_results_p = GetAllMeasuredVariableIds (service_data_p -> dftsd_base_data.sd_service_p);


	if (id_results_p)
		{
			json_t *id_result_p;
			size_t i;
			size_t num_successes = 0;

			json_array_foreach (id_results_p, i, id_result_p)
				{
					bson_oid_t id;

					if (GetMongoIdFromJSON (id_result_p, &id))
						{
							char *id_s = GetBSONOidAsString (&id);

							if (id_s)
								{
									MeasuredVariable *mv_p = GetMeasuredVariableByIdString  (id_s, service_data_p);

									if (mv_p)
										{
											OperationStatus s = IndexMeasuredVariable (mv_p, job_p, id_s, service_data_p);

											FreeMeasuredVariable (mv_p);
										}		/* if (mv_p) */
									else
										{
											PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "GetStudyByIdString () failed for \"%s\"", id_s);
										}
								}		/* if (id_s) */
							else
								{
									PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, id_result_p, "GetBSONOidAsString () failed");
								}

						}		/* if (GetMongoIdFromJSON (id_result_p, &id)) */
					else
						{
							PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, id_result_p, "GetMongoIdFromJSON () failed");
						}

				}		/* json_array_foreach (id_results_p, i, id_result_p) */

			json_decref (id_results_p);
		}		/* if (id_results_p) */


	return status;
}


static ServiceJobSet *RunFieldTrialIndexingService (Service *service_p, ParameterSet *param_set_p, User * UNUSED_PARAM (user_p), ProvidersStateTable * UNUSED_PARAM (providers_p))
{
	FieldTrialServiceData *data_p = (FieldTrialServiceData *) (service_p -> se_data_p);

	service_p->se_jobs_p = AllocateSimpleServiceJobSet (service_p, "RunFieldTrialIndexingService", "DFWFieldTrial");

	if (service_p -> se_jobs_p)
		{
			ServiceJob *job_p = GetServiceJobFromServiceJobSet (service_p -> se_jobs_p, 0);

			SetServiceJobStatus (job_p, OS_IDLE);

			LogParameterSet (param_set_p, job_p);

			if (param_set_p)
				{
					const char *id_s = NULL;
					bool run_flag = false;
					const bool *run_flag_p = &run_flag;
					json_t *terms_p = NULL;

					RunReindexing (param_set_p, job_p, data_p);

					RunCaching (param_set_p, job_p, data_p);


					/*
					 * Generate Frictionless Data packages
					 */
					if (GetCurrentStringParameterValueFromParameterSet (param_set_p, S_GENERATE_FD_PACKAGES.npt_name_s, &id_s))
						{

							if (!IsStringEmpty (id_s))
								{
									OperationStatus fd_status = OS_FAILED_TO_START;

									if (strcmp (id_s, "*") == 0)
										{
											fd_status = GenerateAllFrictionlessDataStudies (job_p, data_p);
										}
									else
										{
											/* reindex the given studies by their ids */
											LinkedList *ids_p = ParseStringToStringLinkedList (id_s, " ", true);
											uint32 num_done = 0;

											if (ids_p)
												{
													StringListNode *node_p = (StringListNode *) (ids_p -> ll_head_p);

													while (node_p)
														{
															const char *study_id_s = node_p -> sln_string_s;
															Study *study_p = GetStudyByIdString (study_id_s, VF_CLIENT_FULL, data_p);

															if (study_p)
																{
																	if (SaveStudyAsFrictionlessData (study_p, data_p))
																		{
																			++ num_done;
																		}
																	else
																		{
																			char *error_s = ConcatenateVarargsStrings ("Could not generate fd package for study with id \"", study_id_s, "\"", NULL);

																			if (error_s)
																				{
																					AddParameterErrorMessageToServiceJob (job_p, S_GENERATE_FD_PACKAGES.npt_name_s, S_GENERATE_FD_PACKAGES.npt_type, error_s);
																					PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, error_s);
																					FreeCopiedString (error_s);
																				}
																			else
																				{
																					const char * const default_error_s = "Could not generate fd package study by id";

																					AddParameterErrorMessageToServiceJob (job_p, S_GENERATE_FD_PACKAGES.npt_name_s, S_GENERATE_FD_PACKAGES.npt_type, default_error_s);
																					PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, default_error_s);
																				}

																		}

																	FreeStudy (study_p);
																}
															else
																{
																	char *error_s = ConcatenateVarargsStrings ("Could not find study with id \"", study_id_s, "\"", NULL);

																	if (error_s)
																		{
																			AddParameterErrorMessageToServiceJob (job_p, S_GENERATE_FD_PACKAGES.npt_name_s, S_GENERATE_FD_PACKAGES.npt_type, error_s);
																			PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, error_s);
																			FreeCopiedString (error_s);
																		}
																	else
																		{
																			const char * const default_error_s = "Could not find study by id";

																			AddParameterErrorMessageToServiceJob (job_p, S_GENERATE_FD_PACKAGES.npt_name_s, S_GENERATE_FD_PACKAGES.npt_type, default_error_s);
																			PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, default_error_s);
																		}


																}

															node_p = (StringListNode *) (node_p -> sln_node.ln_next_p);
														}

													if (num_done == ids_p -> ll_size)
														{
															fd_status = OS_SUCCEEDED;
														}
													else if (num_done > 0)
														{
															fd_status = OS_PARTIALLY_SUCCEEDED;
														}

													FreeLinkedList (ids_p);
												}

										}

									MergeServiceJobStatus (job_p, fd_status);
								}		/* if (!IsStringEmpty (id_s)) */

						}		/* if (GetCurrentStringParameterValueFromParameterSet (param_set_p, S_GENERATE_FD_PACKAGES.npt_name_s, &id_s)) */


					if (GetCurrentStringParameterValueFromParameterSet (param_set_p, S_GENERATE_HANDBOOK.npt_name_s, &id_s))
						{
							if (!IsStringEmpty (id_s))
								{
									const ViewFormat format = VF_CLIENT_MINIMAL;

									if (strcmp (id_s, "*") == 0)
										{
											json_t *studies_json_p = GetAllStudiesAsJSON (data_p, true);

											if (studies_json_p)
												{
													size_t i;
													json_t *study_json_p;

													json_array_foreach (studies_json_p, i, study_json_p)
														{
															Study *study_p = GetStudyFromJSON (study_json_p, format, data_p);

															if (study_p)
																{
																	GenerateStudyHandbook (study_p, job_p, data_p);

																	FreeStudy (study_p);
																}


														}		/* json_array_foreach (studies_json_p, i, study_json_p) */

													json_decref (studies_json_p);
												}
										}
									else
										{
											Study *study_p = GetStudyByIdString (id_s, format, data_p);

											if (study_p)
												{
													GenerateStudyHandbook (study_p, job_p, data_p);

													FreeStudy (study_p);
												}		/*  if (study_p) */

										}

								}		/* if (!IsStringEmpty (id_s)) */
						}


					if (GetCurrentStringParameterValueFromParameterSet (param_set_p, S_REMOVE_STUDY_PLOTS.npt_name_s, &id_s))
						{
							if (!IsStringEmpty (id_s))
								{
									OperationStatus plot_status = RemovePlotsForStudyById (id_s, data_p);

									MergeServiceJobStatus (job_p, plot_status);
								}		/* if (!IsStringEmpty (id_s)) */
						}



					if (GetCurrentBooleanParameterValueFromParameterSet (param_set_p, S_UPDATE_SCALE_TERMS.npt_name_s, &run_flag_p))
						{
							if ((run_flag_p != NULL) && (*run_flag_p == true))
								{
									OperationStatus s = StoreAllScaleUnits (data_p);

									MergeServiceJobStatus (job_p, s);
								}
						}

					if (GetCurrentJSONParameterValueFromParameterSet (param_set_p, S_ROTHAMSTED_TERMS.npt_name_s, (const json_t **) &terms_p))
						{
							if (terms_p)
								{
									const size_t num_rows = json_array_size (terms_p);

									if (num_rows > 0)
										{
											OperationStatus s = StoreAllRResScaleUnits (terms_p, data_p);

											MergeServiceJobStatus (job_p, s);
										}

								}
						}



					/*
					 * Generate Study Statistics
					 */
					if (GetCurrentStringParameterValueFromParameterSet (param_set_p, S_GENERATE_STUDY_METADATA.npt_name_s, &id_s))
						{
							if (!IsStringEmpty (id_s))
								{
									OperationStatus stats_status = OS_FAILED_TO_START;

									if (strcmp (id_s, "*") == 0)
										{
											stats_status = GenerateStatisticsForAllStudies (job_p, data_p);
										}
									else
										{
											/* reindex the given studies by their ids */
											LinkedList *ids_p = ParseStringToStringLinkedList (id_s, " ", true);
											uint32 num_done = 0;

											if (ids_p)
												{
													StringListNode *node_p = (StringListNode *) (ids_p -> ll_head_p);

													while (node_p)
														{
															const char *study_id_s = node_p -> sln_string_s;
															Study *study_p = GetStudyByIdString (study_id_s, VF_CLIENT_FULL, data_p);

															if (study_p)
																{
																	if (GenerateStatisticsForStudy (study_p, job_p, data_p))
																		{
																			++ num_done;
																		}
																	else
																		{
																			char *error_s = ConcatenateVarargsStrings ("Could not generate statistics for study with id \"", study_id_s, "\"", NULL);

																			if (error_s)
																				{
																					AddParameterErrorMessageToServiceJob (job_p, S_GENERATE_STUDY_METADATA.npt_name_s, S_GENERATE_STUDY_METADATA.npt_type, error_s);
																					PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, error_s);
																					FreeCopiedString (error_s);
																				}
																			else
																				{
																					const char * const default_error_s = "Could not generate statistics for study by id";

																					AddParameterErrorMessageToServiceJob (job_p, S_GENERATE_STUDY_METADATA.npt_name_s, S_GENERATE_STUDY_METADATA.npt_type, default_error_s);
																					PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, default_error_s);
																				}

																		}

																	FreeStudy (study_p);
																}
															else
																{
																	char *error_s = ConcatenateVarargsStrings ("Could not find study with id \"", study_id_s, "\"", NULL);

																	if (error_s)
																		{
																			AddParameterErrorMessageToServiceJob (job_p, S_GENERATE_STUDY_METADATA.npt_name_s, S_GENERATE_STUDY_METADATA.npt_type, error_s);
																			PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, error_s);
																			FreeCopiedString (error_s);
																		}
																	else
																		{
																			const char * const default_error_s = "Could not find study by id";

																			AddParameterErrorMessageToServiceJob (job_p, S_GENERATE_STUDY_METADATA.npt_name_s, S_GENERATE_STUDY_METADATA.npt_type, default_error_s);
																			PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, default_error_s);
																		}


																}

															node_p = (StringListNode *) (node_p -> sln_node.ln_next_p);
														}

													if (num_done == ids_p -> ll_size)
														{
															stats_status = OS_SUCCEEDED;
														}
													else if (num_done > 0)
														{
															stats_status = OS_PARTIALLY_SUCCEEDED;
														}

													FreeLinkedList (ids_p);
												}

										}

									MergeServiceJobStatus (job_p, stats_status);

								}		/* if (!IsStringEmpty (id_s)) */

						}


					if (GetCurrentBooleanParameterValueFromParameterSet (param_set_p, S_ADD_MONGODB_INDEXES.npt_name_s, &run_flag_p))
						{
							if ((run_flag_p != NULL) && (*run_flag_p == true))
								{
									OperationStatus s = CreateMongoIndexes (data_p);

									MergeServiceJobStatus (job_p, s);
								}
						}

				}

		}

	return service_p -> se_jobs_p;
}



static void GenerateStudyHandbook (Study *study_p, ServiceJob *job_p, FieldTrialServiceData *data_p)
{
	OperationStatus handbook_status = GenerateStudyAsPDF (study_p, data_p);

	MergeServiceJobStatus (job_p, handbook_status);
}


static OperationStatus StoreAllRResScaleUnits (json_t *terms_p, FieldTrialServiceData *data_p)
{
	OperationStatus status = OS_FAILED;
	MongoTool *mongo_p = GetConfiguredMongoTool (data_p, NULL);

	if (mongo_p)
		{
			if (SetMongoToolCollection (mongo_p, data_p -> dftsd_collection_ss [DFTD_MEASURED_VARIABLE]))
					{
						bson_t *query_p = bson_new ();

						if (query_p)
							{
								size_t i;
								json_t *row_p;
								size_t num_succeeded = 0;
								const size_t num_terms = json_array_size (terms_p);

								json_array_foreach (terms_p, i, row_p)
									{
										const char *unit_id_s = GetJSONString (row_p, "unit-id");

										if (unit_id_s)
											{
												const char *scale_class_s = GetJSONString (row_p, "scale class");

												if (scale_class_s)
													{
														const ScaleClass *scale_class_p = GetScaleClassByName (scale_class_s);

														if (scale_class_p)
															{
																json_t *scale_class_json_p = GetScaleClassAsEmbeddedJSON (scale_class_p, MV_SCALE_S);

																if (scale_class_json_p)
																	{
																		const char * const key_s = "unit.so:sameAs";

																		if (BSON_APPEND_UTF8 (query_p, key_s, unit_id_s))
																			{
																				if (UpdateMongoDocumentsByBSON (mongo_p, query_p, scale_class_json_p, true))
																					{
																						++ num_succeeded;
																					}
																				else
																					{
																						PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, scale_class_json_p, "UpdateMongoDocumentsByBSON () failed for query \"%s\": \"%s\"", key_s, unit_id_s);
																					}

																				bson_init (query_p);
																			}		/* if (BSON_APPEND_UTF8 (query_p, "unit.so:sameAs", unit_id_s)) */
																		else
																			{
																				PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, scale_class_json_p, "BSON_APPEND_UTF8 () failed for query \"%s\": \"%s\"", key_s, unit_id_s);
																			}

																		json_decref (scale_class_json_p);
																	}
																else
																	{
																		PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "GetScaleClassAsJSON () failed for \"%s\"", scale_class_p -> sc_name_s);
																	}
															}
														else
															{
																PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "GetScaleClassByName () failed for \"%s\"", scale_class_s);
															}

													}		/* if (scale_class_s) */
												else
													{
														PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, row_p, "No scale class specified");
													}

											}		/* if (unit_id_s) */
										else
											{
												PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, row_p, "No unit id specified");
											}

									}		/* json_array_foreach (terms_p, i, row_p) */


								if (num_succeeded == num_terms)
									{
										status = OS_SUCCEEDED;
									}
								else if (num_succeeded > 0)
									{
										status = OS_PARTIALLY_SUCCEEDED;
									}

								bson_destroy (query_p);
							}		/* if (query_p) */
						else
							{
								PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to set allocate query");
							}
					}
				else
					{
						PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to set Mongo collection to \"%s\"", data_p -> dftsd_collection_ss [DFTD_MEASURED_VARIABLE]);
					}

			FreeMongoTool (mongo_p);
		}		/* if (mongo_p) */
	else
		{
			PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "GetConfiguredMongoTool () failed");
		}






	return status;
}


static bool GetIndexingParameterTypeForNamedParameter (const Service * UNUSED_PARAM (service_p), const char *param_name_s, ParameterType *pt_p)
{
	const NamedParameterType params [] =
		{
			S_CLEAR_DATA,
			S_REINDEX_ALL_DATA,
			S_REINDEX_TRIALS,
			S_REINDEX_STUDIES,
			S_REINDEX_LOCATIONS,
			S_REINDEX_MEASURED_VARIABLES,
			S_REINDEX_PROGRAMS,
			S_REINDEX_TREATMENTS,
			S_CACHE_CLEAR,
			S_CACHE_LIST,
			S_REMOVE_STUDY_PLOTS,
			S_GENERATE_FD_PACKAGES,
			S_REMOVE_STUDIES,
			S_GENERATE_HANDBOOK,
			S_UPDATE_SCALE_TERMS,
			S_ROTHAMSTED_TERMS,
			S_GENERATE_STUDY_METADATA,
			S_ADD_MONGODB_INDEXES,
			NULL
		};

	return DefaultGetParameterTypeForNamedParameter (param_name_s, pt_p, params);
}


static ParameterSet *GetFieldTrialIndexingServiceParameters (Service *service_p, DataResource * UNUSED_PARAM (resource_p), User * UNUSED_PARAM (user_p))
{
	ParameterSet *params_p = AllocateParameterSet ("Field Trial indexing service parameters", "The parameters used for the Field Trial indexing service");

	if (params_p)
		{
			ServiceData *data_p = service_p -> se_data_p;
			Parameter *param_p = NULL;
			ParameterGroup *indexing_group_p = CreateAndAddParameterGroupToParameterSet ("Indexing", false, data_p, params_p);
			bool b = false;

			if ((param_p = EasyCreateAndAddBooleanParameterToParameterSet (data_p, params_p, indexing_group_p, S_CLEAR_DATA.npt_name_s, "Clear all data", "Clear all data prior to any reindexing", &b, PL_ALL)) != NULL)
				{
					if ((param_p = EasyCreateAndAddBooleanParameterToParameterSet (data_p, params_p, indexing_group_p, S_REINDEX_ALL_DATA.npt_name_s, "Reindex all data", "Reindex all data into Lucene", &b, PL_ALL)) != NULL)
						{
							if ((param_p = EasyCreateAndAddBooleanParameterToParameterSet (data_p, params_p, indexing_group_p,S_REINDEX_TRIALS.npt_name_s, "Reindex all Field Trials", "Reindex all Field Trials into Lucene", &b, PL_ALL)) != NULL)
								{
									if ((param_p = EasyCreateAndAddBooleanParameterToParameterSet (data_p, params_p, indexing_group_p, S_REINDEX_STUDIES.npt_name_s, "Reindex all Studies", "Reindex all Studies into Lucene", &b, PL_ALL)) != NULL)
										{
											if ((param_p = EasyCreateAndAddBooleanParameterToParameterSet (data_p, params_p, indexing_group_p, S_REINDEX_LOCATIONS.npt_name_s, "Reindex all Locations", "Reindex all Locations into Lucene", &b, PL_ALL)) != NULL)
												{
													if ((param_p = EasyCreateAndAddBooleanParameterToParameterSet (data_p, params_p, indexing_group_p, S_REINDEX_MEASURED_VARIABLES.npt_name_s, "Reindex all Measured Variables", "Reindex all Measured Variables into Lucene", &b, PL_ALL)) != NULL)
														{
															if ((param_p = EasyCreateAndAddBooleanParameterToParameterSet (data_p, params_p, indexing_group_p, S_REINDEX_PROGRAMS.npt_name_s, "Reindex all Programmes", "Reindex all Programmes into Lucene", &b, PL_ALL)) != NULL)
																{
																	if ((param_p = EasyCreateAndAddBooleanParameterToParameterSet (data_p, params_p, indexing_group_p, S_REINDEX_TREATMENTS.npt_name_s, "Reindex all Treatments", "Reindex all Treatments into Lucene", &b, PL_ALL)) != NULL)
																		{
																			ParameterGroup *caching_group_p = CreateAndAddParameterGroupToParameterSet ("Cache", false, data_p, params_p);

																			if ((param_p = EasyCreateAndAddStringParameterToParameterSet (data_p, params_p, caching_group_p, S_CACHE_CLEAR.npt_type, S_CACHE_CLEAR.npt_name_s, "Clear Study cache", "Clear any cached Studies with the given Ids. Use * to clear all of them.", NULL, PL_ALL)) != NULL)
																				{
																					if ((param_p = EasyCreateAndAddBooleanParameterToParameterSet (data_p, params_p, caching_group_p, S_CACHE_LIST.npt_name_s, "List cached Studies", "Get the ids and dates of all of the cached Studies", &b, PL_ALL)) != NULL)
																						{
																							ParameterGroup *manager_group_p = CreateAndAddParameterGroupToParameterSet ("Studies", false, data_p, params_p);

																							if ((param_p = EasyCreateAndAddStringParameterToParameterSet (data_p, params_p, manager_group_p, S_GENERATE_FD_PACKAGES.npt_type, S_GENERATE_FD_PACKAGES.npt_name_s, "Generate Frictionless Data Packages", "Generate FD pacakges for given Study Ids. Use * to generate them all.", NULL, PL_ALL)) != NULL)
																								{
																									if ((param_p = EasyCreateAndAddStringParameterToParameterSet (data_p, params_p, manager_group_p, S_REMOVE_STUDY_PLOTS.npt_type, S_REMOVE_STUDY_PLOTS.npt_name_s, "Remove Plots", "Remove all of the Plots for the given Study Id", NULL, PL_ALL)) != NULL)
																										{
																											if ((param_p = EasyCreateAndAddStringParameterToParameterSet (data_p, params_p, caching_group_p, S_GENERATE_HANDBOOK.npt_type, S_GENERATE_HANDBOOK.npt_name_s, "Generate Handbook", "Generate Handbook", NULL, PL_ALL)) != NULL)
																												{
																													if ((param_p = EasyCreateAndAddBooleanParameterToParameterSet (data_p, params_p, manager_group_p, S_UPDATE_SCALE_TERMS.npt_name_s, "Update Scale classes", "Update Scale classes", &b, PL_ALL)) != NULL)
																														{
																															if ((param_p = EasyCreateAndAddJSONParameterToParameterSet (data_p, params_p, manager_group_p, S_ROTHAMSTED_TERMS.npt_type, S_ROTHAMSTED_TERMS.npt_name_s, "Update RRes Scale terms", "Update RRes Scale terms", NULL, PL_ALL)) != NULL)
																																{
																																	AddParameterKeyStringValuePair (param_p, PA_TABLE_ADD_COLUMNS_FLAG_S, "true");

																																	if ((param_p = EasyCreateAndAddJSONParameterToParameterSet (data_p, params_p, manager_group_p, S_GENERATE_STUDY_METADATA.npt_type, S_GENERATE_STUDY_METADATA.npt_name_s, "Generate Metadata", "Generate the Phenotype statistics and accessions cache for the given Study Ids. Use * to generate them all.", NULL, PL_ALL)) != NULL)
																																		{
																																			if ((param_p = EasyCreateAndAddBooleanParameterToParameterSet (data_p, params_p, manager_group_p, S_ADD_MONGODB_INDEXES.npt_name_s, "Add MongoDB Indexes", "Add MongoDB Indexes for faster data handling", &b, PL_ALL)) != NULL)
																																				{
																																					return params_p;
																																				}
																																		}
																																}
																														}
																												}
																										}		
																								}
																						}
																				}
																		}
																}
														}
												}
										}
								}
						}
				}

			FreeParameterSet (params_p);
		}		/* if (params_p) */

	return NULL;
}


static ParameterSet *IsResourceForFieldTrialIndexingService (Service * UNUSED_PARAM (service_p), DataResource * UNUSED_PARAM (resource_p), Handler * UNUSED_PARAM (handler_p))
{
	return NULL;
}


static ServiceMetadata *GetFieldTrialIndexingServiceMetadata (Service *service_p)
{
	const char *term_url_s = CONTEXT_PREFIX_EDAM_ONTOLOGY_S "topic_0625";
	SchemaTerm *category_p = AllocateSchemaTerm (term_url_s, "Genotype and phenotype",
																							 "The study of genetic constitution of a living entity, such as an individual, and organism, a cell and so on, "
																							 "typically with respect to a particular observable phenotypic traits, or resources concerning such traits, which "
																							 "might be an aspect of biochemistry, physiology, morphology, anatomy, development and so on.");

	if (category_p)
		{
			SchemaTerm *subcategory_p;

			term_url_s = CONTEXT_PREFIX_EDAM_ONTOLOGY_S "operation_0304";
			subcategory_p = AllocateSchemaTerm (term_url_s, "Query and retrieval", "Search or query a data resource and retrieve entries and / or annotation.");

			if (subcategory_p)
				{
					ServiceMetadata *metadata_p = AllocateServiceMetadata (category_p, subcategory_p);

					if (metadata_p)
						{
							SchemaTerm *input_p;

							term_url_s = CONTEXT_PREFIX_EDAM_ONTOLOGY_S "data_0968";
							input_p = AllocateSchemaTerm (term_url_s, "Keyword",
																						"Boolean operators (AND, OR and NOT) and wildcard characters may be allowed. Keyword(s) or phrase(s) used (typically) for text-searching purposes.");

							if (input_p)
								{
									if (AddSchemaTermToServiceMetadataInput (metadata_p, input_p))
										{
											SchemaTerm *output_p;

											/* Place */
											term_url_s = CONTEXT_PREFIX_SCHEMA_ORG_S "Place";
											output_p = AllocateSchemaTerm (term_url_s, "Place", "Entities that have a somewhat fixed, physical extension.");

											if (output_p)
												{
													if (AddSchemaTermToServiceMetadataOutput (metadata_p, output_p))
														{
															/* Date */
															term_url_s = CONTEXT_PREFIX_SCHEMA_ORG_S "Date";
															output_p = AllocateSchemaTerm (term_url_s, "Date", "A date value in ISO 8601 date format.");

															if (output_p)
																{
																	if (AddSchemaTermToServiceMetadataOutput (metadata_p, output_p))
																		{
																			/* Pathogen */
																			term_url_s = CONTEXT_PREFIX_EXPERIMENTAL_FACTOR_ONTOLOGY_S "EFO_0000643";
																			output_p = AllocateSchemaTerm (term_url_s, "pathogen", "A biological agent that causes disease or illness to its host.");

																			if (output_p)
																				{
																					if (AddSchemaTermToServiceMetadataOutput (metadata_p, output_p))
																						{
																							/* Phenotype */
																							term_url_s = CONTEXT_PREFIX_EXPERIMENTAL_FACTOR_ONTOLOGY_S "EFO_0000651";
																							output_p = AllocateSchemaTerm (term_url_s, "phenotype", "The observable form taken by some character (or group of characters) "
																																						 "in an individual or an organism, excluding pathology and disease. The detectable outward manifestations of a specific genotype.");

																							if (output_p)
																								{
																									if (AddSchemaTermToServiceMetadataOutput (metadata_p, output_p))
																										{
																											/* Genotype */
																											term_url_s = CONTEXT_PREFIX_EXPERIMENTAL_FACTOR_ONTOLOGY_S "EFO_0000513";
																											output_p = AllocateSchemaTerm (term_url_s, "genotype", "Information, making the distinction between the actual physical material "
																																										 "(e.g. a cell) and the information about the genetic content (genotype).");

																											if (output_p)
																												{
																													if (AddSchemaTermToServiceMetadataOutput (metadata_p, output_p))
																														{
																															return metadata_p;
																														}		/* if (AddSchemaTermToServiceMetadataOutput (metadata_p, output_p)) */
																													else
																														{
																															PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to add output term %s to service metadata", term_url_s);
																															FreeSchemaTerm (output_p);
																														}

																												}		/* if (output_p) */
																											else
																												{
																													PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to allocate output term %s for service metadata", term_url_s);
																												}
																										}		/* if (AddSchemaTermToServiceMetadataOutput (metadata_p, output_p)) */
																									else
																										{
																											PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to add output term %s to service metadata", term_url_s);
																											FreeSchemaTerm (output_p);
																										}

																								}		/* if (output_p) */
																							else
																								{
																									PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to allocate output term %s for service metadata", term_url_s);
																								}

																						}		/* if (AddSchemaTermToServiceMetadataOutput (metadata_p, output_p)) */
																					else
																						{
																							PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to add output term %s to service metadata", term_url_s);
																							FreeSchemaTerm (output_p);
																						}

																				}		/* if (output_p) */
																			else
																				{
																					PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to allocate output term %s for service metadata", term_url_s);
																				}

																		}		/* if (AddSchemaTermToServiceMetadataOutput (metadata_p, output_p)) */
																	else
																		{
																			PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to add output term %s to service metadata", term_url_s);
																			FreeSchemaTerm (output_p);
																		}

																}		/* if (output_p) */
															else
																{
																	PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to allocate output term %s for service metadata", term_url_s);
																}


														}		/* if (AddSchemaTermToServiceMetadataOutput (metadata_p, output_p)) */
													else
														{
															PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to add output term %s to service metadata", term_url_s);
															FreeSchemaTerm (output_p);
														}

												}		/* if (output_p) */
											else
												{
													PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to allocate output term %s for service metadata", term_url_s);
												}

										}		/* if (AddSchemaTermToServiceMetadataInput (metadata_p, input_p)) */
									else
										{
											PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to add input term %s to service metadata", term_url_s);
											FreeSchemaTerm (input_p);
										}

								}		/* if (input_p) */
							else
								{
									PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to allocate input term %s for service metadata", term_url_s);
								}

						}		/* if (metadata_p) */
					else
						{
							PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to allocate service metadata");
						}

				}		/* if (subcategory_p) */
			else
				{
					PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to allocate sub-category term %s for service metadata", term_url_s);
				}

		}		/* if (category_p) */
	else
		{
			PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to allocate category term %s for service metadata", term_url_s);
		}

	return NULL;
}



static void ReleaseFieldTrialIndexingServiceParameters (Service * UNUSED_PARAM (service_p), ParameterSet *params_p)
{
	FreeParameterSet (params_p);
}


static OperationStatus GenerateAllFrictionlessDataStudies (ServiceJob *job_p, FieldTrialServiceData *data_p)
{
	OperationStatus status = OS_IDLE;

	if (data_p -> dftsd_assets_path_s)
		{
			json_t *all_studies_p = GetAllStudiesAsJSON (data_p, true);

			if (all_studies_p)
				{
					json_t *study_json_p;
					size_t i;
					size_t num_saved = 0;
					const size_t num_studies = json_array_size (all_studies_p);

					json_array_foreach (all_studies_p, i, study_json_p)
						{
							Study *study_p = GetStudyFromJSON (study_json_p, VF_CLIENT_FULL, data_p);

							if (study_p)
								{
									if (SaveStudyAsFrictionlessData (study_p, data_p))
										{
											++ num_saved;
										}
									else
										{
											PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, study_json_p, "SaveStudyAsFrictionlessData () failed for \"%s\"", study_p -> st_name_s);
										}

									FreeStudy (study_p);
								}
							else
								{
									PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, study_json_p, "GetStudyFromJSON () failed");
								}
						}

					if (num_saved == num_studies)
						{
							status = OS_SUCCEEDED;
						}
					else if (num_saved > 0)
						{
							status = OS_PARTIALLY_SUCCEEDED;
						}
					else
						{
							status = OS_FAILED;
						}

					json_decref (all_studies_p);
				}		/* if (all_studies_p) */
			else
				{
					PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "GetAllStudiesAsJSON () failed");
					status = OS_FAILED;
				}

		}		/* if (data_p -> dftsd_fd_path_s) */

	return status;
}


static OperationStatus CreateMongoIndexes (FieldTrialServiceData *data_p)
{
	OperationStatus status = OS_FAILED;
	MongoTool *mongo_p = GetConfiguredMongoTool (data_p, NULL);

	if (mongo_p)
		{
			const char *keys_array_ss [5];
			const char **keys_ss = keys_array_ss;

			keys_array_ss [0] = MA_ACCESSION_S;
			keys_array_ss [1] = MA_GENE_BANK_ID_S;
			keys_array_ss [2] = NULL;

			/* Materials */
			if (AddCollectionCompoundIndex (mongo_p, NULL, data_p -> dftsd_collection_ss [DFTD_MATERIAL], keys_ss, true, false))
				{


					keys_array_ss [0] = PL_PARENT_STUDY_S;
					keys_array_ss [1] = PL_ROW_INDEX_S;
					keys_array_ss [2] = PL_COLUMN_INDEX_S;
					keys_array_ss [3] = SR_RACK_INDEX_S;
					keys_array_ss [4] = NULL;

					/* Plots */
					if (AddCollectionCompoundIndex (mongo_p, NULL, data_p -> dftsd_collection_ss [DFTD_PLOT], keys_ss, true, false))
						{
							if (AddCollectionSingleIndex (mongo_p, NULL, data_p -> dftsd_collection_ss [DFTD_PLOT], PL_PARENT_STUDY_S, NULL, false, false))
								{
									uint32 i = 0;
									const uint32 num_keys = 2;
									OperationStatus revisions_status = OS_FAILED;

									/* Measured Variables */
									char *key_s = GetMeasuredVariablesNameKey ();

									if (key_s)
										{
											if (AddCollectionSingleIndex (mongo_p, NULL, data_p -> dftsd_collection_ss [DFTD_MEASURED_VARIABLE], key_s, NULL, true, false))
												{
													++ i;
												}

											FreeMeasuredVariablesNameKey (key_s);
										}

									/* Rows */
									key_s = GetRowsNameKey ();

									if (key_s)
										{
											if (AddCollectionSingleIndex (mongo_p, NULL, data_p -> dftsd_collection_ss [DFTD_PLOT], key_s, NULL, true, false))
												{
													++ i;
												}

											FreeRowsNameKey (key_s);
										}

									status = (i == num_keys) ? OS_SUCCEEDED : OS_PARTIALLY_SUCCEEDED;

									revisions_status = CreateMongoRevisionsCollections (data_p);

									if (revisions_status != OS_SUCCEEDED)
										{
											if (status == OS_SUCCEEDED)
												{
													status = OS_PARTIALLY_SUCCEEDED;
												}
										}
								}

						}

				}		/* if (AddCollectionIndex (tool_p, NULL, data_p -> dftsd_collection_ss [DFTD_MATERIAL], MA_ACCESSION_S, false, false)) */


			FreeMongoTool (mongo_p);
		}		/* if (mongo_p) */
	else
		{
			PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "GetConfiguredMongoTool () failed");
		}





	return status;
}


static OperationStatus CreateMongoRevisionsCollections (FieldTrialServiceData *data_p)
{
	OperationStatus status = OS_FAILED;
	uint32 num_successes = 0;
	MongoTool *mongo_p = GetConfiguredMongoTool (data_p, NULL);

	if (mongo_p)
		{
			for (uint32 i = 0; i < DFTD_NUM_TYPES; ++ i)
				{
					if (CreateMongoRevisionsCollection (mongo_p, data_p -> dftsd_database_s, * ((data_p -> dftsd_backup_collection_ss) + i)))
						{
							++ num_successes;
						}
				}

			FreeMongoTool (mongo_p);
		}		/* if (mongo_p) */
	else
		{
			PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "GetConfiguredMongoTool () failed");
		}


	if (num_successes == DFTD_NUM_TYPES)
		{
			status = OS_SUCCEEDED;
		}
	else if (num_successes > 0)
		{
			status = OS_PARTIALLY_SUCCEEDED;
		}


	return status;
}


static OperationStatus CreateMongoRevisionsCollection (MongoTool *tool_p, const char *database_s, const char *collection_s)
{
	OperationStatus status = OS_FAILED;
	int collection_status = DoesCollectionExist (tool_p, collection_s);

	if ((collection_status == 1) || (CreateMongoToolCollection (tool_p, collection_s, NULL)))
		{
			if (AddCollectionSingleIndex (tool_p, database_s, collection_s, DFT_BACKUPS_ID_KEY_S, NULL, false, false))
				{
					if (AddCollectionSingleIndex (tool_p, database_s, collection_s, MONGO_TIMESTAMP_S, NULL, false, false))
						{
							status = OS_SUCCEEDED;
						}
				}		/* if (AddCollectionSingleIndex (tool_p, database_s, collection_s, DFT_TIMESTAMP_S, false, false)) */
			else
				{
					PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "AddCollectionSingleIndex () failed for \"%s\", \"%s\", \"%s\"", database_s, collection_s, MONGO_TIMESTAMP_S);
				}

		}		/* if (CreateMongoToolCollection (tool_p, collection_s, NULL)) */
	else
		{
			PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "CreateMongoToolCollection () failed for \"%s\"", collection_s);
		}

	return status;
}
