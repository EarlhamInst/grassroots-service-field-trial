/*
 * brapi_importer.c
 *
 *  Created on: 2 Nov 2026
 *      Author: tyrrells
 */

#include <stdlib.h>
#include <string.h>

#include "jansson.h"

#include "brapi_importer.h"

#include "curl_tools.h"
#include "json_util.h"
#include "string_utils.h"
#include "time_util.h"
#include "math_utils.h"
#include "mongodb_tool.h"
#include "address.h"

#include "dfw_util.h"
#include "metadata.h"
#include "person.h"
#include "programme.h"
#include "field_trial.h"
#include "study.h"
#include "location.h"
#include "plot.h"
#include "row.h"
#include "standard_row.h"
#include "row_jobs.h"
#include "material.h"
#include "gene_bank.h"
#include "measured_variable.h"
#include "measured_variable_jobs.h"
#include "observation.h"
#include "observation_metadata.h"



/*
 * Import order:
 *
 * GET programs                                  -> Programme
 *   GET trials?programDbId=                     -> FieldTrial
 *     GET studies?trialDbId=                    -> Study
 *       GET locations/{locationDbId}            -> Location
 *       GET observationunits?studyDbId=         -> Plot + StandardRow
 *       GET observations?studyDbId=             -> Observation
 *
 * The Plots are only saved once all of the Observations have been added
 * to their rows as SavePlot () stores the rows and observations too.
 */


typedef struct BrAPIImporter
{
	CurlTool *bi_curl_p;

	char *bi_url_s;

	GeneBank *bi_gene_bank_p;

	User *bi_user_p;

	ServiceJob *bi_job_p;

	FieldTrialServiceData *bi_data_p;

	/* BrAPI locationDbId -> Grassroots location id string */
	json_t *bi_locations_p;

	/* BrAPI variable names that have no matching Measured Variable */
	json_t *bi_missing_variables_p;

	uint32 bi_num_studies_imported;

	uint32 bi_num_studies_skipped;

	uint32 bi_num_studies_failed;

	uint32 bi_num_plots_imported;

	uint32 bi_num_observations_imported;

	uint32 bi_num_observations_failed;
} BrAPIImporter;


/*
 * A plot-level observationUnit that has been converted to a Grassroots StandardRow.
 */
typedef struct BrAPIUnit
{
	Plot *bu_plot_p;

	StandardRow *bu_row_p;
} BrAPIUnit;


static const uint32 S_BRAPI_PAGE_SIZE = 1000;



/*
 * STATIC DECLARATIONS
 */

static bool InitBrAPIImporter (BrAPIImporter *importer_p, const char * const brapi_url_s, const char * const auth_token_s, const char * const gene_bank_name_s,
															 User *user_p, ServiceJob *job_p, FieldTrialServiceData *data_p);

static void ClearBrAPIImporter (BrAPIImporter *importer_p);

static json_t *GetBrAPIDocument (BrAPIImporter *importer_p, const char * const url_s);

static json_t *GetAllBrAPIPages (BrAPIImporter *importer_p, const char * const endpoint_s, const char * const filter_key_s, const char * const filter_value_s);

static json_t *GetBrAPIResult (BrAPIImporter *importer_p, const char * const endpoint_s, const char * const id_s);

static bool ImportBrAPIProgramme (BrAPIImporter *importer_p, const json_t *brapi_programme_p);

static Programme *GetOrCreateProgramme (BrAPIImporter *importer_p, const json_t *brapi_programme_p);

static bool ImportBrAPITrial (BrAPIImporter *importer_p, const json_t *brapi_trial_p, Programme *programme_p);

static FieldTrial *GetOrCreateFieldTrial (BrAPIImporter *importer_p, const json_t *brapi_trial_p, Programme *programme_p);

static OperationStatus ImportBrAPIStudy (BrAPIImporter *importer_p, const json_t *brapi_study_p, FieldTrial *trial_p);

static Location *GetOrCreateLocation (BrAPIImporter *importer_p, const char * const location_db_id_s, const char * const location_name_s);

static Location *CreateLocationFromBrAPI (BrAPIImporter *importer_p, const char * const location_db_id_s, const char * const location_name_s);

static bool IsPlotObservationUnit (const json_t *unit_p);

static bool GetObservationLevelCode (const json_t *position_p, const char * const level_s, uint32 *value_p);

static bool GetBrAPIUnsignedInt (const json_t *json_p, const char * const key_s, uint32 *value_p);

static bool GetYearFromBrAPIDate (const char * const date_s, uint32 *year_p);

static void GetStudyGridSize (const json_t *units_p, uint32 *num_rows_p, uint32 *num_cols_p);

static bool AddBrAPIPlots (BrAPIImporter *importer_p, Study *study_p, const json_t *units_p, BrAPIUnit *brapi_units_p, json_t *units_map_p);

static bool AddBrAPIPlot (BrAPIImporter *importer_p, Study *study_p, const json_t *unit_p, const uint32 default_index, BrAPIUnit *brapi_unit_p);

static void AddBrAPIObservations (BrAPIImporter *importer_p, const json_t *observations_p, BrAPIUnit *brapi_units_p, const json_t *units_map_p);

static OperationStatus AddBrAPIObservation (BrAPIImporter *importer_p, const json_t *brapi_observation_p, StandardRow *row_p);

static MeasuredVariable *GetMeasuredVariableForBrAPIVariable (BrAPIImporter *importer_p, const char * const variable_s, MEM_FLAG *mv_mem_p);

static void *GetProgrammeObjectFromJSON (const json_t *json_p, const ViewFormat format, const FieldTrialServiceData *data_p);

static void *GetFieldTrialObjectFromJSON (const json_t *json_p, const ViewFormat format, const FieldTrialServiceData *data_p);

static void *GetLocationObjectFromJSON (const json_t *json_p, const ViewFormat format, const FieldTrialServiceData *data_p);

static void AddBrAPIErrorToJob (BrAPIImporter *importer_p, const char * const prefix_s, const char * const name_s);


/*
 * API DEFINITIONS
 */


OperationStatus ImportFromBrAPI (const char * const brapi_url_s, const char * const auth_token_s, const char * const program_db_id_s,
																 const char * const gene_bank_name_s, User *user_p, ServiceJob *job_p, FieldTrialServiceData *data_p)
{
	OperationStatus status = OS_FAILED_TO_START;
	BrAPIImporter importer;

	if (InitBrAPIImporter (&importer, brapi_url_s, auth_token_s, gene_bank_name_s, user_p, job_p, data_p))
		{
			json_t *programmes_p = NULL;

			status = OS_FAILED;

			if (program_db_id_s)
				{
					json_t *programme_p = GetBrAPIResult (&importer, "programs", program_db_id_s);

					if (programme_p)
						{
							programmes_p = json_array ();

							if (programmes_p)
								{
									if (json_array_append_new (programmes_p, programme_p) != 0)
										{
											json_decref (programme_p);
										}
								}
							else
								{
									json_decref (programme_p);
								}
						}
				}
			else
				{
					programmes_p = GetAllBrAPIPages (&importer, "programs", NULL, NULL);
				}

			if (programmes_p)
				{
					size_t i;
					json_t *brapi_programme_p;

					json_array_foreach (programmes_p, i, brapi_programme_p)
						{
							ImportBrAPIProgramme (&importer, brapi_programme_p);
						}

					json_decref (programmes_p);
				}
			else
				{
					PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to get programs from \"%s\"", importer.bi_url_s);
					AddBrAPIErrorToJob (&importer, "Failed to get programs from ", importer.bi_url_s);
				}


			if (json_object_size (importer.bi_missing_variables_p) > 0)
				{
					const char *key_s;
					json_t *value_p;

					json_object_foreach (importer.bi_missing_variables_p, key_s, value_p)
						{
							AddBrAPIErrorToJob (&importer, "No Measured Variable found for BrAPI variable ", key_s);
						}
				}

			PrintLog (STM_LEVEL_INFO, __FILE__, __LINE__, "BrAPI import from \"%s\": " UINT32_FMT " studies imported, " UINT32_FMT " skipped, " UINT32_FMT " failed, "
								UINT32_FMT " plots, " UINT32_FMT " observations imported, " UINT32_FMT " observations failed",
								importer.bi_url_s, importer.bi_num_studies_imported, importer.bi_num_studies_skipped, importer.bi_num_studies_failed,
								importer.bi_num_plots_imported, importer.bi_num_observations_imported, importer.bi_num_observations_failed);

			if (importer.bi_num_studies_imported > 0)
				{
					if ((importer.bi_num_studies_failed == 0) && (importer.bi_num_observations_failed == 0))
						{
							status = OS_SUCCEEDED;
						}
					else
						{
							status = OS_PARTIALLY_SUCCEEDED;
						}
				}
			else if ((importer.bi_num_studies_failed == 0) && (programmes_p))
				{
					/* everything was already imported */
					status = OS_SUCCEEDED;
				}

			ClearBrAPIImporter (&importer);
		}		/* if (InitBrAPIImporter (&importer, brapi_url_s, auth_token_s, gene_bank_name_s, user_p, job_p, data_p)) */

	return status;
}



/*
 * STATIC DEFINITIONS
 */


static bool InitBrAPIImporter (BrAPIImporter *importer_p, const char * const brapi_url_s, const char * const auth_token_s, const char * const gene_bank_name_s,
															 User *user_p, ServiceJob *job_p, FieldTrialServiceData *data_p)
{
	memset (importer_p, 0, sizeof (BrAPIImporter));

	importer_p -> bi_user_p = user_p;
	importer_p -> bi_job_p = job_p;
	importer_p -> bi_data_p = data_p;

	if (IsStringEmpty (brapi_url_s))
		{
			PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "No BrAPI url specified");
			return false;
		}

	if ((importer_p -> bi_url_s = EasyCopyToNewString (brapi_url_s)) != NULL)
		{
			size_t l = strlen (importer_p -> bi_url_s);

			/* remove any trailing slashes so we can add the endpoints consistently */
			while ((l > 0) && (* ((importer_p -> bi_url_s) + l - 1) == '/'))
				{
					-- l;
					* ((importer_p -> bi_url_s) + l) = '\0';
				}

			const char *bank_s = gene_bank_name_s ? gene_bank_name_s : GENE_BANK_GRU_S;

			if ((importer_p -> bi_gene_bank_p = GetGeneBankByName (bank_s, data_p)) != NULL)
				{
					if ((importer_p -> bi_curl_p = AllocateMemoryCurlTool (0)) != NULL)
						{
							bool success_flag = SetCurlToolHeader (importer_p -> bi_curl_p, "Accept", "application/json");

							if (success_flag && (!IsStringEmpty (auth_token_s)))
								{
									char *value_s = ConcatenateStrings ("Bearer ", auth_token_s);

									success_flag = false;

									if (value_s)
										{
											success_flag = SetCurlToolHeader (importer_p -> bi_curl_p, "Authorization", value_s);
											FreeCopiedString (value_s);
										}
								}

							if (success_flag)
								{
									if ((importer_p -> bi_locations_p = json_object ()) != NULL)
										{
											if ((importer_p -> bi_missing_variables_p = json_object ()) != NULL)
												{
													return true;
												}
										}
								}
						}
					else
						{
							PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to allocate CurlTool");
						}
				}
			else
				{
					PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to find Gene Bank \"%s\"", bank_s);
					AddBrAPIErrorToJob (importer_p, "Failed to find Gene Bank ", bank_s);
				}
		}

	ClearBrAPIImporter (importer_p);

	return false;
}


static void ClearBrAPIImporter (BrAPIImporter *importer_p)
{
	if (importer_p -> bi_missing_variables_p)
		{
			json_decref (importer_p -> bi_missing_variables_p);
			importer_p -> bi_missing_variables_p = NULL;
		}

	if (importer_p -> bi_locations_p)
		{
			json_decref (importer_p -> bi_locations_p);
			importer_p -> bi_locations_p = NULL;
		}

	if (importer_p -> bi_curl_p)
		{
			FreeCurlTool (importer_p -> bi_curl_p);
			importer_p -> bi_curl_p = NULL;
		}

	if (importer_p -> bi_gene_bank_p)
		{
			FreeGeneBank (importer_p -> bi_gene_bank_p);
			importer_p -> bi_gene_bank_p = NULL;
		}

	if (importer_p -> bi_url_s)
		{
			FreeCopiedString (importer_p -> bi_url_s);
			importer_p -> bi_url_s = NULL;
		}
}


static json_t *GetBrAPIDocument (BrAPIImporter *importer_p, const char * const url_s)
{
	json_t *doc_p = NULL;

	if (SetUriForCurlTool (importer_p -> bi_curl_p, url_s))
		{
			CURLcode res;

			ClearCurlToolData (importer_p -> bi_curl_p);

			res = RunCurlTool (importer_p -> bi_curl_p);

			if (res == CURLE_OK)
				{
					const char *data_s = GetCurlToolData (importer_p -> bi_curl_p);

					if (data_s)
						{
							json_error_t err;

							doc_p = json_loads (data_s, 0, &err);

							if (!doc_p)
								{
									PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to parse response from \"%s\", error at line %d, column %d: %s", url_s, err.line, err.column, err.text);
								}
						}
					else
						{
							PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "No data returned from \"%s\"", url_s);
						}
				}
			else
				{
					PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to call \"%s\": %s", url_s, curl_easy_strerror (res));
				}
		}
	else
		{
			PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to set CurlTool url to \"%s\"", url_s);
		}

	return doc_p;
}


/*
 * Get all of the entries for a BrAPI list call, e.g. "studies?trialDbId=1",
 * across all of its pages as a single array.
 */
static json_t *GetAllBrAPIPages (BrAPIImporter *importer_p, const char * const endpoint_s, const char * const filter_key_s, const char * const filter_value_s)
{
	json_t *results_p = json_array ();

	if (results_p)
		{
			char *filter_s = NULL;
			char *page_size_s = ConvertUnsignedIntegerToString (S_BRAPI_PAGE_SIZE);
			uint32 page = 0;
			uint32 total_pages = 1;
			bool success_flag = (page_size_s != NULL);

			if (success_flag && filter_key_s)
				{
					char *escaped_value_s = GetURLEscapedString (importer_p -> bi_curl_p, filter_value_s);

					success_flag = false;

					if (escaped_value_s)
						{
							if ((filter_s = ConcatenateVarargsStrings (filter_key_s, "=", escaped_value_s, "&", NULL)) != NULL)
								{
									success_flag = true;
								}

							FreeURLEscapedString (escaped_value_s);
						}
				}

			while (success_flag && (page < total_pages))
				{
					char *page_s = ConvertUnsignedIntegerToString (page);

					success_flag = false;

					if (page_s)
						{
							char *url_s = ConcatenateVarargsStrings (importer_p -> bi_url_s, "/", endpoint_s, "?", filter_s ? filter_s : "", "page=", page_s, "&pageSize=", page_size_s, NULL);

							if (url_s)
								{
									json_t *doc_p = GetBrAPIDocument (importer_p, url_s);

									if (doc_p)
										{
											const json_t *result_p = json_object_get (doc_p, "result");
											json_t *data_p = result_p ? json_object_get (result_p, "data") : NULL;

											if (json_is_array (data_p))
												{
													const json_t *metadata_p = json_object_get (doc_p, "metadata");
													const json_t *pagination_p = metadata_p ? json_object_get (metadata_p, "pagination") : NULL;

													if (json_array_extend (results_p, data_p) == 0)
														{
															success_flag = true;

															if (pagination_p)
																{
																	if (!GetBrAPIUnsignedInt (pagination_p, "totalPages", &total_pages))
																		{
																			total_pages = 1;
																		}
																}

															/* Guard against servers that report the wrong number of pages */
															if (json_array_size (data_p) == 0)
																{
																	total_pages = 0;
																}

															++ page;
														}
												}
											else
												{
													PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, doc_p, "No result.data array from \"%s\"", url_s);
												}

											json_decref (doc_p);
										}

									FreeCopiedString (url_s);
								}

							FreeCopiedString (page_s);
						}
				}		/* while (success_flag && (page < total_pages)) */

			if (filter_s)
				{
					FreeCopiedString (filter_s);
				}

			if (page_size_s)
				{
					FreeCopiedString (page_size_s);
				}

			if (!success_flag)
				{
					json_decref (results_p);
					results_p = NULL;
				}
		}

	return results_p;
}


/*
 * Get the "result" object for a single BrAPI object e.g. "locations/1"
 */
static json_t *GetBrAPIResult (BrAPIImporter *importer_p, const char * const endpoint_s, const char * const id_s)
{
	json_t *result_p = NULL;
	char *escaped_id_s = GetURLEscapedString (importer_p -> bi_curl_p, id_s);

	if (escaped_id_s)
		{
			char *url_s = ConcatenateVarargsStrings (importer_p -> bi_url_s, "/", endpoint_s, "/", escaped_id_s, NULL);

			if (url_s)
				{
					json_t *doc_p = GetBrAPIDocument (importer_p, url_s);

					if (doc_p)
						{
							json_t *value_p = json_object_get (doc_p, "result");

							if (json_is_object (value_p))
								{
									result_p = json_incref (value_p);
								}
							else
								{
									PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, doc_p, "No result object from \"%s\"", url_s);
								}

							json_decref (doc_p);
						}

					FreeCopiedString (url_s);
				}

			FreeURLEscapedString (escaped_id_s);
		}

	return result_p;
}


static bool ImportBrAPIProgramme (BrAPIImporter *importer_p, const json_t *brapi_programme_p)
{
	bool success_flag = false;
	const char *programme_db_id_s = GetJSONString (brapi_programme_p, "programDbId");

	if (programme_db_id_s)
		{
			Programme *programme_p = GetOrCreateProgramme (importer_p, brapi_programme_p);

			if (programme_p)
				{
					json_t *trials_p = GetAllBrAPIPages (importer_p, "trials", "programDbId", programme_db_id_s);

					if (trials_p)
						{
							size_t i;
							json_t *brapi_trial_p;

							success_flag = true;

							json_array_foreach (trials_p, i, brapi_trial_p)
								{
									if (!ImportBrAPITrial (importer_p, brapi_trial_p, programme_p))
										{
											success_flag = false;
										}
								}

							json_decref (trials_p);
						}
					else
						{
							AddBrAPIErrorToJob (importer_p, "Failed to get trials for program ", programme_p -> pr_name_s);
						}

					FreeProgramme (programme_p);
				}
		}
	else
		{
			PrintJSONToErrors (STM_LEVEL_WARNING, __FILE__, __LINE__, brapi_programme_p, "BrAPI program has no programDbId");
		}

	return success_flag;
}


static Programme *GetOrCreateProgramme (BrAPIImporter *importer_p, const json_t *brapi_programme_p)
{
	Programme *programme_p = NULL;
	const char *name_s = GetJSONString (brapi_programme_p, "programName");

	if (IsStringEmpty (name_s))
		{
			PrintJSONToErrors (STM_LEVEL_WARNING, __FILE__, __LINE__, brapi_programme_p, "BrAPI program has no programName");
			return NULL;
		}

	programme_p = (Programme *) GetDFWObjectByNamedStringField (PR_NAME_S, name_s, DFTD_PROGRAMME, GetProgrammeObjectFromJSON, VF_STORAGE, importer_p -> bi_data_p);

	if (!programme_p)
		{
			const char *pi_name_s = GetJSONString (brapi_programme_p, "leadPersonName");
			Person *pi_p = AllocatePerson (IsStringEmpty (pi_name_s) ? "Unknown" : pi_name_s, NULL, NULL, NULL, NULL);

			if (pi_p)
				{
					Metadata *metadata_p = AllocateMetadata (NULL, importer_p -> bi_user_p, false, NULL);

					if (metadata_p)
						{
							const char *abbreviation_s = GetJSONString (brapi_programme_p, "abbreviation");
							const char *documentation_url_s = GetJSONString (brapi_programme_p, "documentationURL");
							const char *objective_s = GetJSONString (brapi_programme_p, "objective");

							programme_p = AllocateProgramme (NULL, metadata_p, abbreviation_s, NULL, documentation_url_s, name_s, objective_s, pi_p, NULL, NULL, NULL);

							if (programme_p)
								{
									OperationStatus s = SaveProgramme (programme_p, importer_p -> bi_job_p, importer_p -> bi_data_p);

									if ((s != OS_SUCCEEDED) && (s != OS_PARTIALLY_SUCCEEDED))
										{
											PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to save Programme \"%s\"", name_s);
											AddBrAPIErrorToJob (importer_p, "Failed to save Programme ", name_s);

											FreeProgramme (programme_p);
											programme_p = NULL;
										}
								}
							else
								{
									PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to allocate Programme \"%s\"", name_s);
									FreeMetadata (metadata_p);
									FreePerson (pi_p);
								}
						}
					else
						{
							FreePerson (pi_p);
						}
				}
		}		/* if (!programme_p) */

	return programme_p;
}


static bool ImportBrAPITrial (BrAPIImporter *importer_p, const json_t *brapi_trial_p, Programme *programme_p)
{
	bool success_flag = false;
	const char *trial_db_id_s = GetJSONString (brapi_trial_p, "trialDbId");

	if (trial_db_id_s)
		{
			FieldTrial *trial_p = GetOrCreateFieldTrial (importer_p, brapi_trial_p, programme_p);

			if (trial_p)
				{
					json_t *studies_p = GetAllBrAPIPages (importer_p, "studies", "trialDbId", trial_db_id_s);

					if (studies_p)
						{
							size_t i;
							json_t *brapi_study_p;

							success_flag = true;

							json_array_foreach (studies_p, i, brapi_study_p)
								{
									OperationStatus s = ImportBrAPIStudy (importer_p, brapi_study_p, trial_p);

									if (s == OS_SUCCEEDED)
										{
											++ (importer_p -> bi_num_studies_imported);
										}
									else if (s == OS_IDLE)
										{
											++ (importer_p -> bi_num_studies_skipped);
										}
									else
										{
											++ (importer_p -> bi_num_studies_failed);
											success_flag = false;
										}
								}

							json_decref (studies_p);
						}
					else
						{
							AddBrAPIErrorToJob (importer_p, "Failed to get studies for trial ", trial_p -> ft_name_s);
						}

					FreeFieldTrial (trial_p);
				}
		}
	else
		{
			PrintJSONToErrors (STM_LEVEL_WARNING, __FILE__, __LINE__, brapi_trial_p, "BrAPI trial has no trialDbId");
		}

	return success_flag;
}


static FieldTrial *GetOrCreateFieldTrial (BrAPIImporter *importer_p, const json_t *brapi_trial_p, Programme *programme_p)
{
	FieldTrial *trial_p = NULL;
	const char *name_s = GetJSONString (brapi_trial_p, "trialName");

	if (IsStringEmpty (name_s))
		{
			PrintJSONToErrors (STM_LEVEL_WARNING, __FILE__, __LINE__, brapi_trial_p, "BrAPI trial has no trialName");
			return NULL;
		}

	trial_p = (FieldTrial *) GetDFWObjectByNamedStringField (FT_NAME_S, name_s, DFTD_FIELD_TRIAL, GetFieldTrialObjectFromJSON, VF_STORAGE, importer_p -> bi_data_p);

	if (!trial_p)
		{
			/* The Programme is freed by the caller after all of its trials have been imported */
			trial_p = AllocateFieldTrial (name_s, NULL, programme_p, MF_ALREADY_FREED, NULL, NULL);

			if (trial_p)
				{
					if (!SaveFieldTrial (trial_p, importer_p -> bi_job_p, importer_p -> bi_data_p))
						{
							PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to save Field Trial \"%s\"", name_s);
							AddBrAPIErrorToJob (importer_p, "Failed to save Field Trial ", name_s);

							FreeFieldTrial (trial_p);
							trial_p = NULL;
						}
				}
			else
				{
					PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to allocate Field Trial \"%s\"", name_s);
				}
		}

	return trial_p;
}


/*
 * Returns OS_SUCCEEDED if the study was imported, OS_IDLE if it
 * already exists and was skipped, and OS_FAILED otherwise.
 */
static OperationStatus ImportBrAPIStudy (BrAPIImporter *importer_p, const json_t *brapi_study_p, FieldTrial *trial_p)
{
	OperationStatus status = OS_FAILED;
	const char *study_db_id_s = GetJSONString (brapi_study_p, "studyDbId");
	const char *name_s = GetJSONString (brapi_study_p, "studyName");
	Study *study_p = NULL;
	json_t *units_p = NULL;

	if ((IsStringEmpty (study_db_id_s)) || (IsStringEmpty (name_s)))
		{
			PrintJSONToErrors (STM_LEVEL_WARNING, __FILE__, __LINE__, brapi_study_p, "BrAPI study has no studyDbId or studyName");
			return OS_FAILED;
		}

	/* Don't import the same study twice */
	study_p = (Study *) GetDFWObjectByNamedStringField (ST_NAME_S, name_s, DFTD_STUDY, GetStudyCallback, VF_STORAGE, importer_p -> bi_data_p);

	if (study_p)
		{
			PrintLog (STM_LEVEL_INFO, __FILE__, __LINE__, "Study \"%s\" already exists, skipping BrAPI study \"%s\"", name_s, study_db_id_s);
			FreeStudy (study_p);
			return OS_IDLE;
		}

	units_p = GetAllBrAPIPages (importer_p, "observationunits", "studyDbId", study_db_id_s);

	if (units_p)
		{
			Location *location_p = GetOrCreateLocation (importer_p, GetJSONString (brapi_study_p, "locationDbId"), GetJSONString (brapi_study_p, "locationName"));
			Metadata *metadata_p = AllocateMetadata (NULL, importer_p -> bi_user_p, false, NULL);

			if (metadata_p)
				{
					const json_t *design_p = json_object_get (brapi_study_p, "experimentalDesign");
					const json_t *facility_p = json_object_get (brapi_study_p, "growthFacility");
					const char *design_s = design_p ? GetJSONString (design_p, "description") : NULL;
					const char *growing_conditions_s = facility_p ? GetJSONString (facility_p, "description") : NULL;
					const char *data_url_s = GetJSONString (brapi_study_p, "documentationURL");
					const char *description_s = GetJSONString (brapi_study_p, "studyDescription");
					uint32 sowing_year = 0;
					uint32 harvest_year = 0;
					uint32 num_rows = 0;
					uint32 num_cols = 0;
					const uint32 *sowing_year_p = GetYearFromBrAPIDate (GetJSONString (brapi_study_p, "startDate"), &sowing_year) ? &sowing_year : NULL;
					const uint32 *harvest_year_p = GetYearFromBrAPIDate (GetJSONString (brapi_study_p, "endDate"), &harvest_year) ? &harvest_year : NULL;

					if ((!design_s) && design_p)
						{
							design_s = GetJSONString (design_p, "PUI");
						}

					GetStudyGridSize (units_p, &num_rows, &num_cols);

					/* The Study takes ownership of the metadata and location */
					study_p = AllocateStudy (NULL, metadata_p, name_s, data_url_s, NULL, NULL,
																	 location_p, trial_p,
																	 MF_ALREADY_FREED, NULL, NULL, description_s,
																	 design_s, growing_conditions_s, NULL,
																	 (num_rows > 0) ? &num_rows : NULL, (num_cols > 0) ? &num_cols : NULL, NULL, NULL, NULL,
																	 NULL, NULL, NULL, NULL,
																	 NULL, NULL, NULL,
																	 NULL,
																	 NULL, NULL,
																	 sowing_year_p, harvest_year_p,
																	 NULL, NULL, NULL,
																	 NULL, NULL,
																	 NULL,
																	 NULL,
																	 importer_p -> bi_data_p);

					if (study_p)
						{
							location_p = NULL;

							status = SaveStudy (study_p, importer_p -> bi_job_p, importer_p -> bi_data_p, NULL);

							if ((status == OS_SUCCEEDED) || (status == OS_PARTIALLY_SUCCEEDED))
								{
									const size_t num_units = json_array_size (units_p);
									BrAPIUnit *brapi_units_p = num_units > 0 ? (BrAPIUnit *) AllocMemoryArray (num_units, sizeof (BrAPIUnit)) : NULL;
									json_t *units_map_p = json_object ();

									status = OS_FAILED;

									if (((num_units == 0) || brapi_units_p) && units_map_p)
										{
											if (AddBrAPIPlots (importer_p, study_p, units_p, brapi_units_p, units_map_p))
												{
													json_t *observations_p = NULL;
													size_t i;
													bool plots_saved_flag = true;

													if (num_units > 0)
														{
															observations_p = GetAllBrAPIPages (importer_p, "observations", "studyDbId", study_db_id_s);

															if (observations_p)
																{
																	AddBrAPIObservations (importer_p, observations_p, brapi_units_p, units_map_p);
																	json_decref (observations_p);
																}
															else
																{
																	AddBrAPIErrorToJob (importer_p, "Failed to get observations for study ", name_s);
																}
														}

													/* Now the rows have all of their observations, save the plots */
													for (i = 0; i < num_units; ++ i)
														{
															Plot *plot_p = (brapi_units_p + i) -> bu_plot_p;

															if (plot_p)
																{
																	if (SavePlot (plot_p, importer_p -> bi_data_p))
																		{
																			++ (importer_p -> bi_num_plots_imported);
																		}
																	else
																		{
																			PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "SavePlot () failed for [" UINT32_FMT ", " UINT32_FMT "] in study \"%s\"",
																									 plot_p -> pl_row_index, plot_p -> pl_column_index, name_s);
																			plots_saved_flag = false;
																		}
																}
														}

													if (plots_saved_flag && observations_p)
														{
															status = OS_SUCCEEDED;
														}
													else if (num_units == 0)
														{
															status = OS_SUCCEEDED;
														}
													else
														{
															AddBrAPIErrorToJob (importer_p, "Failed to import all plots for study ", name_s);
														}
												}
											else
												{
													AddBrAPIErrorToJob (importer_p, "Failed to create plots for study ", name_s);
												}
										}

									if (units_map_p)
										{
											json_decref (units_map_p);
										}

									if (brapi_units_p)
										{
											FreeMemory (brapi_units_p);
										}
								}
							else
								{
									PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to save Study \"%s\"", name_s);
									AddBrAPIErrorToJob (importer_p, "Failed to save Study ", name_s);
									status = OS_FAILED;
								}

							/* This frees all of the study's plots too */
							FreeStudy (study_p);
						}
					else
						{
							PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to allocate Study \"%s\"", name_s);
							FreeMetadata (metadata_p);
						}
				}

			if (location_p)
				{
					FreeLocation (location_p);
				}

			json_decref (units_p);
		}
	else
		{
			AddBrAPIErrorToJob (importer_p, "Failed to get observation units for study ", name_s);
		}

	return status;
}


/*
 * Get a newly-allocated Location for a Study to own. Locations are matched
 * by name so that the same location isn't stored more than once.
 */
static Location *GetOrCreateLocation (BrAPIImporter *importer_p, const char * const location_db_id_s, const char * const location_name_s)
{
	Location *location_p = NULL;

	if (IsStringEmpty (location_db_id_s))
		{
			return NULL;
		}

	{
		const char *id_s = GetJSONString (importer_p -> bi_locations_p, location_db_id_s);

		if (id_s)
			{
				location_p = GetLocationByIdString (id_s, VF_STORAGE, importer_p -> bi_data_p);
			}
	}

	if (!location_p)
		{
			if (!IsStringEmpty (location_name_s))
				{
					location_p = (Location *) GetDFWObjectByNamedStringField (LO_NAME_S, location_name_s, DFTD_LOCATION, GetLocationObjectFromJSON, VF_STORAGE, importer_p -> bi_data_p);
				}

			if (!location_p)
				{
					location_p = CreateLocationFromBrAPI (importer_p, location_db_id_s, location_name_s);
				}

			if (location_p)
				{
					char *id_s = GetBSONOidAsString (location_p -> lo_id_p);

					if (id_s)
						{
							if (!SetJSONString (importer_p -> bi_locations_p, location_db_id_s, id_s))
								{
									PrintErrors (STM_LEVEL_WARNING, __FILE__, __LINE__, "Failed to cache location id \"%s\" for \"%s\"", id_s, location_db_id_s);
								}

							FreeBSONOidString (id_s);
						}
				}
		}

	return location_p;
}


static Location *CreateLocationFromBrAPI (BrAPIImporter *importer_p, const char * const location_db_id_s, const char * const location_name_s)
{
	Location *location_p = NULL;
	json_t *brapi_location_p = GetBrAPIResult (importer_p, "locations", location_db_id_s);
	const char *name_s = location_name_s;
	const char *country_s = NULL;
	const char *country_code_s = NULL;

	if (brapi_location_p)
		{
			const char *value_s = GetJSONString (brapi_location_p, "locationName");

			if (!IsStringEmpty (value_s))
				{
					name_s = value_s;
				}

			country_s = GetJSONString (brapi_location_p, "countryName");
			country_code_s = GetJSONString (brapi_location_p, "countryCode");
		}

	if (!IsStringEmpty (name_s))
		{
			Address *address_p = AllocateAddress (name_s, NULL, NULL, NULL, country_s, NULL, country_code_s, NULL);

			if (address_p)
				{
					/*
					 * The coordinates are GeoJSON so are in the order
					 * longitude, latitude and an optional altitude
					 */
					if (brapi_location_p)
						{
							const json_t *coords_p = json_object_get (brapi_location_p, "coordinates");
							const json_t *geometry_p = coords_p ? json_object_get (coords_p, "geometry") : NULL;
							const json_t *point_p = geometry_p ? json_object_get (geometry_p, "coordinates") : NULL;

							if (json_is_array (point_p) && (json_array_size (point_p) >= 2))
								{
									const json_t *lon_p = json_array_get (point_p, 0);
									const json_t *lat_p = json_array_get (point_p, 1);

									if (json_is_number (lon_p) && json_is_number (lat_p))
										{
											const json_t *alt_p = json_array_get (point_p, 2);
											double64 alt = 0.0;
											const double64 *alt_p_value = NULL;

											if (json_is_number (alt_p))
												{
													alt = json_number_value (alt_p);
													alt_p_value = &alt;
												}

											if (!SetAddressCentreCoordinate (address_p, json_number_value (lat_p), json_number_value (lon_p), alt_p_value))
												{
													PrintErrors (STM_LEVEL_WARNING, __FILE__, __LINE__, "Failed to set coordinates for location \"%s\"", name_s);
												}
										}
								}
						}

					/* The Location takes ownership of the Address */
					location_p = AllocateLocation (address_p, 0, NULL, NULL, NULL, LT_UNKNOWN, NULL);

					if (location_p)
						{
							OperationStatus s = SaveLocation (location_p, importer_p -> bi_job_p, importer_p -> bi_data_p);

							if ((s != OS_SUCCEEDED) && (s != OS_PARTIALLY_SUCCEEDED))
								{
									PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to save Location \"%s\"", name_s);
									AddBrAPIErrorToJob (importer_p, "Failed to save Location ", name_s);

									FreeLocation (location_p);
									location_p = NULL;
								}
						}
					else
						{
							PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to allocate Location \"%s\"", name_s);
							FreeAddress (address_p);
						}
				}
		}
	else
		{
			PrintErrors (STM_LEVEL_WARNING, __FILE__, __LINE__, "No name for BrAPI location \"%s\"", location_db_id_s);
		}

	if (brapi_location_p)
		{
			json_decref (brapi_location_p);
		}

	return location_p;
}


/*
 * BrAPI observation units can be at any level such as plot, plant or subplot.
 * We only import plots, or units that don't say what level they are.
 */
static bool IsPlotObservationUnit (const json_t *unit_p)
{
	const json_t *position_p = json_object_get (unit_p, "observationUnitPosition");

	if (position_p)
		{
			const json_t *level_p = json_object_get (position_p, "observationLevel");

			if (level_p)
				{
					const char *level_s = GetJSONString (level_p, "levelName");

					if (level_s)
						{
							return (Stricmp (level_s, "plot") == 0);
						}
				}
		}

	return true;
}


/*
 * Get the code of a given level such as "plot", "rep" or "block" from an
 * observationUnitPosition. This is either the unit's own level or one of
 * its observationLevelRelationships.
 */
static bool GetObservationLevelCode (const json_t *position_p, const char * const level_s, uint32 *value_p)
{
	const json_t *level_p = json_object_get (position_p, "observationLevel");
	const json_t *relationships_p = json_object_get (position_p, "observationLevelRelationships");

	if (level_p)
		{
			const char *name_s = GetJSONString (level_p, "levelName");

			if (name_s && (Stricmp (name_s, level_s) == 0))
				{
					return GetBrAPIUnsignedInt (level_p, "levelCode", value_p);
				}
		}

	if (json_is_array (relationships_p))
		{
			size_t i;
			const json_t *relationship_p;

			json_array_foreach (relationships_p, i, relationship_p)
				{
					const char *name_s = GetJSONString (relationship_p, "levelName");

					if (name_s && (Stricmp (name_s, level_s) == 0))
						{
							return GetBrAPIUnsignedInt (relationship_p, "levelCode", value_p);
						}
				}
		}

	return false;
}


/*
 * BrAPI often sends numbers as strings, e.g. "positionCoordinateX": "3",
 * so accept either.
 */
static bool GetBrAPIUnsignedInt (const json_t *json_p, const char * const key_s, uint32 *value_p)
{
	const json_t *value_json_p = json_object_get (json_p, key_s);

	if (json_is_integer (value_json_p))
		{
			json_int_t i = json_integer_value (value_json_p);

			if (i >= 0)
				{
					*value_p = (uint32) i;
					return true;
				}
		}
	else if (json_is_string (value_json_p))
		{
			const char *value_s = json_string_value (value_json_p);
			char *end_s = NULL;
			unsigned long l = strtoul (value_s, &end_s, 10);

			if ((end_s != value_s) && (*end_s == '\0'))
				{
					*value_p = (uint32) l;
					return true;
				}
		}

	return false;
}


static bool GetYearFromBrAPIDate (const char * const date_s, uint32 *year_p)
{
	if (date_s && (strlen (date_s) >= 10))
		{
			struct tm t;

			memset (&t, 0, sizeof (struct tm));

			/* BrAPI dates are ISO 8601 so the first 10 characters are YYYY-MM-DD */
			if (SetTimeFromString (&t, date_s))
				{
					*year_p = (uint32) (t.tm_year + 1900);
					return true;
				}
		}

	return false;
}


static void GetStudyGridSize (const json_t *units_p, uint32 *num_rows_p, uint32 *num_cols_p)
{
	size_t i;
	const json_t *unit_p;

	json_array_foreach (units_p, i, unit_p)
		{
			if (IsPlotObservationUnit (unit_p))
				{
					const json_t *position_p = json_object_get (unit_p, "observationUnitPosition");

					if (position_p)
						{
							uint32 value;

							if (GetBrAPIUnsignedInt (position_p, "positionCoordinateY", &value) && (value > *num_rows_p))
								{
									*num_rows_p = value;
								}

							if (GetBrAPIUnsignedInt (position_p, "positionCoordinateX", &value) && (value > *num_cols_p))
								{
									*num_cols_p = value;
								}
						}
				}
		}
}


static bool AddBrAPIPlots (BrAPIImporter *importer_p, Study *study_p, const json_t *units_p, BrAPIUnit *brapi_units_p, json_t *units_map_p)
{
	size_t i;
	const json_t *unit_p;
	uint32 num_plots = 0;
	uint32 num_failed = 0;

	json_array_foreach (units_p, i, unit_p)
		{
			if (IsPlotObservationUnit (unit_p))
				{
					const char *unit_db_id_s = GetJSONString (unit_p, "observationUnitDbId");

					if (unit_db_id_s)
						{
							BrAPIUnit *brapi_unit_p = brapi_units_p + i;

							/* Plot numbers are 1-based */
							if (AddBrAPIPlot (importer_p, study_p, unit_p, num_plots + 1, brapi_unit_p))
								{
									++ num_plots;

									if (json_object_set_new (units_map_p, unit_db_id_s, json_integer ((json_int_t) i)) != 0)
										{
											PrintErrors (STM_LEVEL_WARNING, __FILE__, __LINE__, "Failed to store observation unit \"%s\"", unit_db_id_s);
										}
								}
							else
								{
									++ num_failed;
								}
						}
				}
		}

	return (num_failed == 0);
}


static bool AddBrAPIPlot (BrAPIImporter *importer_p, Study *study_p, const json_t *unit_p, const uint32 default_index, BrAPIUnit *brapi_unit_p)
{
	bool success_flag = false;
	const json_t *position_p = json_object_get (unit_p, "observationUnitPosition");
	const char *germplasm_s = GetJSONString (unit_p, "germplasmName");
	const char *unit_name_s = GetJSONString (unit_p, "observationUnitName");
	uint32 row = default_index;
	uint32 column = 1;
	uint32 plot_number = default_index;
	uint32 replicate = 1;
	Plot *plot_p = NULL;

	if (position_p)
		{
			uint32 value;

			if (GetBrAPIUnsignedInt (position_p, "positionCoordinateY", &value))
				{
					row = value;
				}

			if (GetBrAPIUnsignedInt (position_p, "positionCoordinateX", &value))
				{
					column = value;
				}

			if (GetObservationLevelCode (position_p, "plot", &value))
				{
					plot_number = value;
				}

			if (GetObservationLevelCode (position_p, "rep", &value) || GetObservationLevelCode (position_p, "replicate", &value))
				{
					replicate = value;
				}
		}

	if (IsStringEmpty (germplasm_s))
		{
			PrintJSONToErrors (STM_LEVEL_WARNING, __FILE__, __LINE__, unit_p, "BrAPI observation unit has no germplasmName in study \"%s\"", study_p -> st_name_s);
			return false;
		}

	/* Use the BrAPI unit name as the comment so the plot can be traced back to the source */
	plot_p = AllocatePlot (NULL, NULL, NULL, NULL, NULL, row, column, NULL, unit_name_s, NULL, NULL, study_p);

	if (plot_p)
		{
			Material *material_p = GetOrCreateMaterialByAccession (germplasm_s, NULL, importer_p -> bi_gene_bank_p, importer_p -> bi_data_p);

			if (material_p)
				{
					StandardRow *sr_p = AllocateStandardRow (NULL, 1, plot_number, false, replicate, material_p, MF_SHALLOW_COPY, NULL, plot_p);

					if (sr_p)
						{
							if (AddRowToPlot (plot_p, & (sr_p -> sr_base)))
								{
									if (AddPlotToStudy (study_p, plot_p))
										{
											brapi_unit_p -> bu_plot_p = plot_p;
											brapi_unit_p -> bu_row_p = sr_p;
											success_flag = true;
										}
									else
										{
											PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "AddPlotToStudy () failed for [" UINT32_FMT ", " UINT32_FMT "] in study \"%s\"", row, column, study_p -> st_name_s);
										}
								}
							else
								{
									PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "AddRowToPlot () failed for [" UINT32_FMT ", " UINT32_FMT "] in study \"%s\"", row, column, study_p -> st_name_s);
									FreeRow (& (sr_p -> sr_base));
								}
						}
					else
						{
							PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "AllocateStandardRow () failed for [" UINT32_FMT ", " UINT32_FMT "] in study \"%s\"", row, column, study_p -> st_name_s);
							FreeMaterial (material_p);
						}
				}
			else
				{
					PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to get Material for \"%s\" in study \"%s\"", germplasm_s, study_p -> st_name_s);
				}

			if (!success_flag)
				{
					FreePlot (plot_p);
				}
		}
	else
		{
			PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "AllocatePlot () failed for [" UINT32_FMT ", " UINT32_FMT "] in study \"%s\"", row, column, study_p -> st_name_s);
		}

	return success_flag;
}


static void AddBrAPIObservations (BrAPIImporter *importer_p, const json_t *observations_p, BrAPIUnit *brapi_units_p, const json_t *units_map_p)
{
	size_t i;
	const json_t *brapi_observation_p;

	json_array_foreach (observations_p, i, brapi_observation_p)
		{
			const char *unit_db_id_s = GetJSONString (brapi_observation_p, "observationUnitDbId");

			if (unit_db_id_s)
				{
					const json_t *index_p = json_object_get (units_map_p, unit_db_id_s);

					/*
					 * Observations on units that we didn't import, e.g. plants, are ignored
					 */
					if (json_is_integer (index_p))
						{
							StandardRow *row_p = (brapi_units_p + json_integer_value (index_p)) -> bu_row_p;
							OperationStatus s = AddBrAPIObservation (importer_p, brapi_observation_p, row_p);

							if ((s == OS_SUCCEEDED) || (s == OS_PARTIALLY_SUCCEEDED))
								{
									++ (importer_p -> bi_num_observations_imported);
								}
							else if (s != OS_IDLE)
								{
									++ (importer_p -> bi_num_observations_failed);
								}
						}
				}
		}
}


/*
 * Returns OS_IDLE if the observation was skipped because it has no
 * value or no matching Measured Variable.
 */
static OperationStatus AddBrAPIObservation (BrAPIImporter *importer_p, const json_t *brapi_observation_p, StandardRow *row_p)
{
	OperationStatus status = OS_IDLE;
	const char *variable_s = GetJSONString (brapi_observation_p, "observationVariableName");
	const char *value_s = GetJSONString (brapi_observation_p, "value");

	if ((!IsStringEmpty (variable_s)) && (!IsStringEmpty (value_s)))
		{
			MEM_FLAG mv_mem = MF_ALREADY_FREED;
			MeasuredVariable *mv_p = GetMeasuredVariableForBrAPIVariable (importer_p, variable_s, &mv_mem);

			if (mv_p)
				{
					const char *timestamp_s = GetJSONString (brapi_observation_p, "observationTimeStamp");
					struct tm date;
					struct tm *date_p = NULL;
					ObservationMetadata *metadata_p = NULL;

					if (timestamp_s && (strlen (timestamp_s) >= 10))
						{
							memset (&date, 0, sizeof (struct tm));

							if (SetTimeFromString (&date, timestamp_s))
								{
									date_p = &date;
								}
						}

					status = OS_FAILED;
					metadata_p = AllocateObservationMetadata (date_p, NULL, false, OB_DEFAULT_INDEX);

					if (metadata_p)
						{
							json_t *raw_value_p = json_string (value_s);

							if (raw_value_p)
								{
									bool free_measured_variable_flag = false;

									status = AddObservationValueToStandardRowByParts (importer_p -> bi_job_p, row_p, mv_p, metadata_p, variable_s, raw_value_p, NULL, NULL,
																																		&free_measured_variable_flag, NULL, NULL);

									/*
									 * If the Observation wasn't created, we need to free the Measured
									 * Variable as FreeObservation () would normally take care of that.
									 */
									if (free_measured_variable_flag && (mv_mem != MF_SHADOW_USE))
										{
											if (!HasMeasuredVariableCache (importer_p -> bi_data_p))
												{
													FreeMeasuredVariable (mv_p);
												}
										}

									json_decref (raw_value_p);
								}

							FreeObservationMetadata (metadata_p);
						}
				}		/* if (mv_p) */

		}		/* if ((!IsStringEmpty (variable_s)) && (!IsStringEmpty (value_s))) */

	return status;
}


/*
 * BreedBase names its variables as "<trait name>|<ontology id>", e.g.
 * "grain yield|CO_321:0000013", so as well as the full name, try matching
 * each half of it.
 */
static MeasuredVariable *GetMeasuredVariableForBrAPIVariable (BrAPIImporter *importer_p, const char * const variable_s, MEM_FLAG *mv_mem_p)
{
	MeasuredVariable *mv_p = NULL;

	if (json_object_get (importer_p -> bi_missing_variables_p, variable_s))
		{
			return NULL;
		}

	mv_p = GetMeasuredVariableByVariableName (variable_s, mv_mem_p, importer_p -> bi_data_p);

	if (!mv_p)
		{
			const char *separator_s = strchr (variable_s, '|');

			if (separator_s)
				{
					char *trait_s = CopyToNewString (variable_s, separator_s - variable_s, false);

					if (!IsStringEmpty (separator_s + 1))
						{
							mv_p = GetMeasuredVariableByVariableName (separator_s + 1, mv_mem_p, importer_p -> bi_data_p);
						}

					if (trait_s)
						{
							if ((!mv_p) && (!IsStringEmpty (trait_s)))
								{
									mv_p = GetMeasuredVariableByVariableName (trait_s, mv_mem_p, importer_p -> bi_data_p);
								}

							FreeCopiedString (trait_s);
						}
				}
		}

	if (!mv_p)
		{
			if (json_object_set_new (importer_p -> bi_missing_variables_p, variable_s, json_true ()) != 0)
				{
					PrintErrors (STM_LEVEL_WARNING, __FILE__, __LINE__, "Failed to store missing variable \"%s\"", variable_s);
				}
		}

	return mv_p;
}


static void *GetProgrammeObjectFromJSON (const json_t *json_p, const ViewFormat format, const FieldTrialServiceData *data_p)
{
	return GetProgrammeFromJSON (json_p, format, data_p);
}


static void *GetFieldTrialObjectFromJSON (const json_t *json_p, const ViewFormat format, const FieldTrialServiceData *data_p)
{
	return GetFieldTrialFromJSON (json_p, format, data_p);
}


static void *GetLocationObjectFromJSON (const json_t *json_p, const ViewFormat UNUSED_PARAM (format), const FieldTrialServiceData *data_p)
{
	return GetLocationFromJSON (json_p, data_p);
}


static void AddBrAPIErrorToJob (BrAPIImporter *importer_p, const char * const prefix_s, const char * const name_s)
{
	if (importer_p -> bi_job_p)
		{
			char *error_s = ConcatenateVarargsStrings (prefix_s, "\"", name_s ? name_s : "", "\"", NULL);

			if (error_s)
				{
					AddGeneralErrorMessageToServiceJob (importer_p -> bi_job_p, error_s);
					FreeCopiedString (error_s);
				}
			else
				{
					AddGeneralErrorMessageToServiceJob (importer_p -> bi_job_p, prefix_s);
				}
		}
}
