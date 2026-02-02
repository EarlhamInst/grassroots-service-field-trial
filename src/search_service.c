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
 * search_service.c
 *
 *  Created on: 24 Oct 2018
 *      Author: billy
 */

#include "search_service.h"
#include "plot_jobs.h"
#include "field_trial_jobs.h"
#include "study_jobs.h"
#include "measured_variable_jobs.h"
#include "material_jobs.h"
#include "location_jobs.h"
#include "gene_bank_jobs.h"
#include "programme_jobs.h"
#include "material_jobs.h"
#include "treatment_jobs.h"
#include "dfw_util.h"


#include "boolean_parameter.h"
#include "unsigned_int_parameter.h"
#include "string_parameter.h"

#include "audit.h"
#include "streams.h"
#include "math_utils.h"
#include "string_utils.h"

#include "lucene_tool.h"
#include "key_value_pair.h"

/*
 * Static declarations
 */


static NamedParameterType S_KEYWORD = { "FT Keyword Search", PT_KEYWORD };
static NamedParameterType S_FACET = { "FT Facet", PT_STRING };
static NamedParameterType S_PAGE_NUMBER = { "FT Results Page Number", PT_UNSIGNED_INT };
static NamedParameterType S_PAGE_SIZE = { "FT Results Page Size", PT_UNSIGNED_INT };

static NamedParameterType S_FACET_STUDY = { "FT Study Facet", PT_BOOLEAN };
static NamedParameterType S_FACET_FIELD_TRIAL = { "FT Trial Facet", PT_BOOLEAN };
static NamedParameterType S_FACET_VARIABLE = { "FT Measured Variable", PT_BOOLEAN };
static NamedParameterType S_FACET_LOCATION = { "FT Location Facet", PT_BOOLEAN };
static NamedParameterType S_FACET_PROGRAMME = { "FT Programme", PT_BOOLEAN };
static NamedParameterType S_FACET_TREATMENT = { "FT Treatment", PT_BOOLEAN };


static const char * const S_ANY_FACET_S = "<ANY>";
static const char * const S_FIELD_TRIAL_FACET_S = "Field Trial";
static const char * const S_STUDY_FACET_S = "Study";
static const char * const S_VARIABLE_FACET_S = "Measured Variable";
static const char * const S_LOCATION_FACET_S = "Location";
static const char * const S_PROGRAMME_FACET_S = "Programme";
static const char * const S_TREATMENT_FACET_S = "Treatment";

static const uint32 S_DEFAULT_PAGE_NUMBER = 0;
static const uint32 S_DEFAULT_PAGE_SIZE = 10;


static const char *GetDFWFieldTrialSearchServiceName (const Service *service_p);

static const char *GetDFWFieldTrialSearchServiceDescription (const Service *service_p);

static const char *GetDFWFieldTrialSearchServiceAlias (const Service *service_p);

static const char *GetDFWFieldTrialSearchServiceInformationUri (const Service *service_p);

static ParameterSet *GetDFWFieldTrialSearchServiceParameters (Service *service_p, DataResource *resource_p, User *user_p);

static bool GetDFWFieldTrialSearchServiceParameterTypesForNamedParameters (const Service *service_p, const char *param_name_s, ParameterType *pt_p);


static void ReleaseDFWFieldTrialSearchServiceParameters (Service *service_p, ParameterSet *params_p);

static ServiceJobSet *RunDFWFieldTrialSearchService (Service *service_p, ParameterSet *param_set_p, User *user_p, ProvidersStateTable *providers_p);

static ParameterSet *IsResourceForDFWFieldTrialSearchService (Service *service_p, DataResource *resource_p, Handler *handler_p);

static bool CloseDFWFieldTrialSearchService (Service *service_p);

static ServiceMetadata *GetDFWFieldTrialSearchServiceMetadata (Service *service_p);


static void SearchFieldTrialsForKeyword (const char *keyword_s, LinkedList *facets_p, const uint32 page_number, const uint32 page_size, ServiceJob *job_p, const ViewFormat fmt, FieldTrialServiceData *data_p);


static bool AddFieldTrialResultsFromLuceneResults (const json_t *document_p, const uint32 index, void *data_p);


static Parameter *AddFacetParameter (ParameterSet *params_p, ParameterGroup *group_p, FieldTrialServiceData *data_p);

static bool AddFacetParameters (ParameterSet *params_p, ParameterGroup *group_p, FieldTrialServiceData *data_p);

static bool AddFacetParameterToList (const char *name_s, const char *value_s, ParameterSet *params_p, LinkedList *facets_p);

static LinkedList *GetFacets (ParameterSet *params_p);


typedef struct
{
	FieldTrialServiceData *sd_service_data_p;
	ServiceJob *sd_job_p;
	ViewFormat sd_format;
} SearchData;


/*
 * API definitions
 */


Service *GetDFWFieldTrialSearchService (GrassrootsServer *grassroots_p)
{
	Service *service_p = (Service *) AllocMemory (sizeof (Service));

	if (service_p)
		{
			FieldTrialServiceData *data_p = AllocateFieldTrialServiceData ();

			if (data_p)
				{
					if (InitialiseService (service_p,
														 GetDFWFieldTrialSearchServiceName,
														 GetDFWFieldTrialSearchServiceDescription,
														 GetDFWFieldTrialSearchServiceAlias,
														 GetDFWFieldTrialSearchServiceInformationUri,
														 RunDFWFieldTrialSearchService,
														 IsResourceForDFWFieldTrialSearchService,
														 GetDFWFieldTrialSearchServiceParameters,
														 GetDFWFieldTrialSearchServiceParameterTypesForNamedParameters,
														 ReleaseDFWFieldTrialSearchServiceParameters,
														 CloseDFWFieldTrialSearchService,
														 NULL,
														 false,
														 SY_SYNCHRONOUS,
														 (ServiceData *) data_p,
														 GetDFWFieldTrialSearchServiceMetadata,
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



static const char *GetDFWFieldTrialSearchServiceName (const Service * UNUSED_PARAM (service_p))
{
	return "Search Field Trials";
}


static const char *GetDFWFieldTrialSearchServiceDescription (const Service * UNUSED_PARAM (service_p))
{
	return "A service to search field trial data";
}


static const char *GetDFWFieldTrialSearchServiceAlias (const Service * UNUSED_PARAM (service_p))
{
	return DFT_GROUP_ALIAS_PREFIX_S SERVICE_GROUP_ALIAS_SEPARATOR "search";
}


static const char *GetDFWFieldTrialSearchServiceInformationUri (const Service *service_p)
{
	const char *url_s = GetServiceInformationPage (service_p);

	if (!url_s)
		{
			url_s = "https://grassroots.tools/docs/user/services/field_trial/search_portal.md";
		}

	return url_s;
}


static Parameter *AddFacetParameter (ParameterSet *params_p, ParameterGroup *group_p, FieldTrialServiceData *data_p)
{
	Parameter *param_p = EasyCreateAndAddStringParameterToParameterSet (& (data_p -> dftsd_base_data), params_p, group_p, S_FACET.npt_type, S_FACET.npt_name_s, "Type", "The type of data to search for", S_ANY_FACET_S, PL_ALL);

	if (param_p)
		{
			if (CreateAndAddStringParameterOption (param_p, S_ANY_FACET_S, "Any"))
				{
					if (CreateAndAddStringParameterOption (param_p, S_FIELD_TRIAL_FACET_S, S_FIELD_TRIAL_FACET_S))
						{
							if (CreateAndAddStringParameterOption (param_p, S_STUDY_FACET_S, S_STUDY_FACET_S))
								{
									if (CreateAndAddStringParameterOption (param_p, S_VARIABLE_FACET_S, S_VARIABLE_FACET_S))
										{
											if (CreateAndAddStringParameterOption (param_p, S_LOCATION_FACET_S, S_LOCATION_FACET_S))
												{
													if (CreateAndAddStringParameterOption (param_p, S_TREATMENT_FACET_S, S_TREATMENT_FACET_S))
														{
															return param_p;
														}
												}
										}
								}
						}
				}
		}

	return NULL;
}


static bool AddFacetParameters (ParameterSet *params_p, ParameterGroup *group_p, FieldTrialServiceData *data_p)
{
	bool b = false;

	if (EasyCreateAndAddBooleanParameterToParameterSet (& (data_p -> dftsd_base_data), params_p, group_p, S_FACET_STUDY.npt_name_s, "Study", "Search across all Studies", &b, PL_ALL))
		{
			if (EasyCreateAndAddBooleanParameterToParameterSet (& (data_p -> dftsd_base_data), params_p, group_p, S_FACET_FIELD_TRIAL.npt_name_s, "Field Trial", "Search across all Trials", &b, PL_ALL))
				{
					if (EasyCreateAndAddBooleanParameterToParameterSet (& (data_p -> dftsd_base_data), params_p, group_p, S_FACET_LOCATION.npt_name_s, "Location", "Search across all Locations", &b, PL_ALL))
						{
							if (EasyCreateAndAddBooleanParameterToParameterSet (& (data_p -> dftsd_base_data), params_p, group_p, S_FACET_VARIABLE.npt_name_s, "Measured Variable", "Search across all Measured Variables", &b, PL_ALL))
								{
									if (EasyCreateAndAddBooleanParameterToParameterSet (& (data_p -> dftsd_base_data), params_p, group_p, S_FACET_PROGRAMME.npt_name_s, "Programme", "Search across all Programmes", &b, PL_ALL))
										{
											if (EasyCreateAndAddBooleanParameterToParameterSet (& (data_p -> dftsd_base_data), params_p, group_p, S_FACET_TREATMENT.npt_name_s, "Treatment", "Search across all Treatments", &b, PL_ALL))
												{
													return true;
												}
										}
								}
						}
				}
		}

	return false;
}



static ParameterSet *GetDFWFieldTrialSearchServiceParameters (Service *service_p, DataResource * UNUSED_PARAM (resource_p), User * UNUSED_PARAM (user_p))
{
	ParameterSet *params_p = AllocateParameterSet ("DFWFieldTrial search service parameters", "The parameters used for the DFWFieldTrial search service");

	if (params_p)
		{
			FieldTrialServiceData *data_p = (FieldTrialServiceData *) service_p -> se_data_p;
			ParameterGroup *group_p = NULL;
			Parameter *param_p = NULL;

			if ((param_p = EasyCreateAndAddStringParameterToParameterSet (& (data_p -> dftsd_base_data), params_p, group_p, S_KEYWORD.npt_type, S_KEYWORD.npt_name_s, "Search", "Search the field trial data", NULL, PL_SIMPLE)) != NULL)
				{
					if (AddFacetParameters (params_p, group_p, data_p))
						{
							uint32 def = S_DEFAULT_PAGE_NUMBER;

							if ((param_p = EasyCreateAndAddUnsignedIntParameterToParameterSet (& (data_p -> dftsd_base_data), params_p, group_p, S_PAGE_NUMBER.npt_name_s, "Page", "The number of the results page to get", &def, PL_SIMPLE)) != NULL)
								{
									def = S_DEFAULT_PAGE_SIZE;

									if ((param_p = EasyCreateAndAddUnsignedIntParameterToParameterSet (& (data_p -> dftsd_base_data), params_p, group_p, S_PAGE_SIZE.npt_name_s, "Page size", "The maximum number of results on each page", &def, PL_SIMPLE)) != NULL)
										{
											if (AddSearchFieldTrialParams (& (data_p -> dftsd_base_data), params_p))
												{
													if (AddSearchStudyParams (& (data_p -> dftsd_base_data), params_p))
														{
															if (AddSearchPlotParams (& (data_p -> dftsd_base_data), params_p))
																{
																	if (AddSearchLocationParams (& (data_p -> dftsd_base_data), params_p))
																		{
																			if (AddSearchMaterialParams (& (data_p -> dftsd_base_data), params_p))
																				{
																					return params_p;
																				}
																			else
																				{
																					PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "AddSearchMaterialParams failed");
																				}
																		}
																	else
																		{
																			PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "AddSearchLocationParams failed");
																		}

																}
															else
																{
																	PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "AddSearchPlotParams failed");
																}
														}
													else
														{
															PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "AddSearchStudyParams failed");
														}
												}
											else
												{
													PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "AddSearchFieldTrialParams failed");
												}

										}		/* if ((param_p = EasyCreateAndAddParameterToParameterSet (& (data_p -> dftsd_base_data), params_p, group_p, S_PAGE_SIZE.npt_type, S_PAGE_SIZE.npt_name_s, "Page size", "The maximum number of results on each page", def, PL_SIMPLE)) != NULL) */
									else
										{
											PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to add %s parameter", S_PAGE_SIZE.npt_name_s);
										}

								}		/* if ((param_p = EasyCreateAndAddParameterToParameterSet (& (data_p -> dftsd_base_data), params_p, group_p, S_PAGE_NUMBER.npt_type, S_PAGE_NUMBER.npt_name_s, "Page", "The page of results to get", def, PL_SIMPLE)) != NULL) */
							else
								{
									PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to add %s parameter", S_PAGE_NUMBER.npt_name_s);
								}

						}		/* if (AddFacetParameter (params_p, group_p, data_p)) */
					else
						{
							PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to add Facet parameter");
						}

				}		/* if ((param_p = EasyCreateAndAddParameterToParameterSet (data_p, params_p, NULL, S_KEYWORD.npt_type, S_KEYWORD.npt_name_s, "Search", "Search the field trial data", def, PL_SIMPLE)) != NULL) */
			else
				{
					PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to add %s parameter", S_KEYWORD.npt_name_s);
				}


			FreeParameterSet (params_p);
		}
	else
		{
			PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to allocate %s ParameterSet", GetDFWFieldTrialSearchServiceName (service_p));
		}

	return NULL;
}


static bool GetDFWFieldTrialSearchServiceParameterTypesForNamedParameters (const Service *service_p, const char *param_name_s, ParameterType *pt_p)
{
	bool success_flag = true;

	const NamedParameterType params [] =
		{
			S_KEYWORD,
			S_FACET,
			S_PAGE_NUMBER,
			S_PAGE_SIZE,
			S_FACET_STUDY,
			S_FACET_FIELD_TRIAL,
			S_FACET_LOCATION,
			S_FACET_VARIABLE,
			S_FACET_PROGRAMME,
			S_FACET_TREATMENT,
			NULL
		};

	if (!DefaultGetParameterTypeForNamedParameter (param_name_s, pt_p, params))
		{
			if (!GetSearchFieldTrialParameterTypeForNamedParameter (param_name_s, pt_p))
				{
					if (!GetSearchStudyParameterTypeForNamedParameter (param_name_s, pt_p))
						{
							if (!GetSearchPlotParameterTypeForNamedParameter (param_name_s, pt_p))
								{
									if (!GetSearchLocationParameterTypeForNamedParameter (param_name_s, pt_p))
										{
											if (!GetSearchMaterialParameterTypeForNamedParameter (param_name_s, pt_p))
												{
													if (!GetSearchProgrammeParameterTypeForNamedParameter (param_name_s, pt_p))
														{
															success_flag = false;
														}
												}		/* if (!GetSearchMaterialParameterTypeForNamedParameter (param_name_s, pt_p)) */

										}		/* if (!GetSearchLocationParameterTypeForNamedParameter (param_name_s, pt_p)) */

								}		/* if (!GetSearchPlotParameterTypeForNamedParameter (param_name_s, pt_p)) */

						}		/* if (!GetSearchStudyParameterTypeForNamedParameter (param_name_s, pt_p)) */

				}		/* if (!GetSearchFieldTrialParameterTypeForNamedParameter (param_name_s, pt_p)) */
		}

	return success_flag;
}



static void ReleaseDFWFieldTrialSearchServiceParameters (Service * UNUSED_PARAM (service_p), ParameterSet *params_p)
{
	FreeParameterSet (params_p);
}


static bool CloseDFWFieldTrialSearchService (Service *service_p)
{
	bool success_flag = true;

	FreeFieldTrialServiceData ((FieldTrialServiceData *) (service_p -> se_data_p));;

	return success_flag;
}


static bool AddFacetParameterToList (const char *name_s, const char *value_s, ParameterSet *params_p, LinkedList *facets_p)
{
	bool success_flag = true;
	const bool *b_p = NULL;

	GetCurrentBooleanParameterValueFromParameterSet (params_p, name_s, &b_p);

	if (b_p && (*b_p))
		{
			StringListNode *node_p = AllocateStringListNode (value_s, MF_SHADOW_USE);

			if (node_p)
				{
					LinkedListAddTail (facets_p, & (node_p -> sln_node));
				}
			else
				{
					success_flag = false;
				}
		}

	return success_flag;
}


static LinkedList *GetFacets (ParameterSet *params_p)
{
	LinkedList *facets_p = AllocateStringLinkedList ();

	if (facets_p)
		{
			if (AddFacetParameterToList (S_FACET_FIELD_TRIAL.npt_name_s, S_FIELD_TRIAL_FACET_S, params_p, facets_p))
				{
					if (AddFacetParameterToList (S_FACET_STUDY.npt_name_s, S_STUDY_FACET_S, params_p, facets_p))
						{
							if (AddFacetParameterToList (S_FACET_LOCATION.npt_name_s, S_LOCATION_FACET_S, params_p, facets_p))
								{
									if (AddFacetParameterToList (S_FACET_VARIABLE.npt_name_s, S_VARIABLE_FACET_S, params_p, facets_p))
										{
											if (AddFacetParameterToList (S_FACET_PROGRAMME.npt_name_s, S_PROGRAMME_FACET_S, params_p, facets_p))
												{
													if (AddFacetParameterToList (S_FACET_TREATMENT.npt_name_s, S_TREATMENT_FACET_S, params_p, facets_p))
														{
															return facets_p;
														}
												}
										}
								}

						}

				}

			FreeLinkedList (facets_p);
		}

	return NULL;
}


static ServiceJobSet *RunDFWFieldTrialSearchService (Service *service_p, ParameterSet *param_set_p, User * UNUSED_PARAM (user_p), ProvidersStateTable * UNUSED_PARAM (providers_p))
{
	FieldTrialServiceData *data_p = (FieldTrialServiceData *) (service_p -> se_data_p);

	service_p -> se_jobs_p = AllocateSimpleServiceJobSet (service_p, NULL, "Field Trial Search");

	if (service_p -> se_jobs_p)
		{
			ServiceJob *job_p = GetServiceJobFromServiceJobSet (service_p -> se_jobs_p, 0);

			LogParameterSet (param_set_p, job_p);

			SetServiceJobStatus (job_p, OS_FAILED_TO_START);

			if (param_set_p)
				{
					/*
					 * check for simple search first
					 */
					if (param_set_p -> ps_current_level & PL_SIMPLE)
						{
							LinkedList *facets_p = GetFacets (param_set_p);

							if (facets_p)
								{
									const char *keyword_s = NULL;
									const uint32 *page_number_p = NULL;
									const uint32 *page_size_p = NULL;

									GetCurrentStringParameterValueFromParameterSet (param_set_p, S_KEYWORD.npt_name_s, &keyword_s);

									GetCurrentUnsignedIntParameterValueFromParameterSet (param_set_p, S_PAGE_NUMBER.npt_name_s, &page_number_p);
									GetCurrentUnsignedIntParameterValueFromParameterSet (param_set_p, S_PAGE_SIZE.npt_name_s, &page_size_p);

									SearchFieldTrialsForKeyword (keyword_s, facets_p, page_number_p ? *page_number_p : S_DEFAULT_PAGE_NUMBER, page_size_p ? *page_size_p : S_DEFAULT_PAGE_SIZE, job_p, VF_CLIENT_MINIMAL, data_p);

									FreeLinkedList (facets_p);
								}

						}
					else
						{
							/*
							 * check for the advanced search
							 */
							if (!RunForSearchFieldTrialParams (data_p, param_set_p, job_p))
								{
									if (!RunForAdvancedSearchStudyParams (data_p, param_set_p, job_p))
										{
											if (!RunForSearchPlotParams (data_p, param_set_p, job_p))
												{
													if (!RunForSearchLocationParams (data_p, param_set_p, job_p))
														{
															if (!RunForSearchMaterialParams (data_p, param_set_p, job_p))
																{
																	if (!RunForSearchProgrammeParams (data_p, param_set_p, job_p))
																		{

																		}		/* if (!RunForSearchProgrammeParams (data_p, param_set_p, job_p)) */

																}		/* if (!RunForSearchMaterialParams (data_p, param_set_p, job_p)) */

														}		/* if (!RunForLocationParams (data_p, param_set_p, job_p)) */

												}

										}		/* if (!RunForStudyParams (data_p, param_set_p, job_p)) */

								}		/* if (!RunForFieldTrialParams (data_p, param_set_p, job_p)) */

						}		/* if (!ran_flag) */

				}		/* if (param_set_p) */

#if DFW_FIELD_TRIAL_SERVICE_DEBUG >= STM_LEVEL_FINE
			PrintJSONToLog (STM_LEVEL_FINE, __FILE__, __LINE__, job_p -> sj_metadata_p, "metadata 3: ");
#endif

			LogServiceJob (job_p);
		}		/* if (service_p -> se_jobs_p) */

	return service_p -> se_jobs_p;
}


static ServiceMetadata *GetDFWFieldTrialSearchServiceMetadata (Service * UNUSED_PARAM (service_p))
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


static ParameterSet *IsResourceForDFWFieldTrialSearchService (Service * UNUSED_PARAM (service_p), DataResource * UNUSED_PARAM (resource_p), Handler * UNUSED_PARAM (handler_p))
{
	return NULL;
}


static void SearchFieldTrialsForKeyword (const char *keyword_s, LinkedList *facet_values_p, const uint32 page_number, const uint32 page_size, ServiceJob *job_p, const ViewFormat fmt, FieldTrialServiceData *data_p)
{
	OperationStatus status = OS_FAILED_TO_START;
	GrassrootsServer *grassroots_p = GetGrassrootsServerFromService (data_p -> dftsd_base_data.sd_service_p);
	LuceneTool *lucene_p = AllocateLuceneTool (grassroots_p, job_p -> sj_id);

	if (lucene_p)
		{
			LinkedList *facets_p = NULL;
			bool success_flag = true;

			/*
			 * Get all the facets specified
			 */
			if (facet_values_p)
				{
					facets_p = AllocateLinkedList (FreeKeyValuePairNode);

					if (facets_p)
						{
							StringListNode *facet_str_p = (StringListNode *) (facet_values_p -> ll_head_p);

							while (facet_str_p && success_flag)
								{
									KeyValuePairNode *facet_p = AllocateKeyValuePairNodeByParts (lucene_p -> lt_facet_key_s, facet_str_p -> sln_string_s);

									if (facet_p)
										{
											LinkedListAddTail (facets_p, & (facet_p -> kvpn_node));

											facet_str_p = (StringListNode *) (facet_str_p -> sln_node.ln_next_p);
										}		/* if (facet_p) */
									else
										{
											PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "AllocateKeyValuePairNode for facet \"type\": \"%s\" failed", facet_str_p -> sln_string_s);
											success_flag = false;
										}

								}
						}		/* if (facets_p) */
					else
						{
							PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "AllocateLinkedList for facets failed");
							success_flag = false;
						}

				}		/* if (facet_values_p) */


			if (success_flag)
				{
					if (SetLuceneToolName (lucene_p, "search_keywords"))
						{
							if (SearchLucene (lucene_p, keyword_s, facets_p, "drill-down", page_number, page_size, QM_PARSER))
								{
									SearchData sd;
									const uint32 from = page_number * page_size;
									const uint32 to = from + page_size - 1;

									sd.sd_service_data_p = data_p;
									sd.sd_job_p = job_p;
									sd.sd_format = fmt;

									status = ParseLuceneResults (lucene_p, from, to, AddFieldTrialResultsFromLuceneResults, &sd);

									if ((status == OS_SUCCEEDED) || (status == OS_PARTIALLY_SUCCEEDED))
										{
											bool added_metadata_flag = false;
											json_error_t error;
											json_t *metadata_p = json_pack_ex (&error, 0, "{s:i,s:i,s:i}",
																												 LT_NUM_TOTAL_HITS_S, lucene_p -> lt_num_total_hits,
																												 LT_HITS_START_INDEX_S, lucene_p -> lt_hits_from_index,
																												 LT_HITS_END_INDEX_S, lucene_p -> lt_hits_to_index);

											if (metadata_p)
												{
													if (AddLuceneFacetResultsToJSON (lucene_p, metadata_p))
														{
															added_metadata_flag = true;
														}

													job_p -> sj_metadata_p = metadata_p;
												}
											else
												{
													PrintErrors (STM_LEVEL_WARNING, __FILE__, __LINE__, "Failed to create metadata for lucene hits");
												}

											if ((!added_metadata_flag) && (status == OS_SUCCEEDED))
												{
													status = OS_PARTIALLY_SUCCEEDED;
												}

										}		/* if ((status == OS_SUCCEEDED) || (status == OS_PARTIALLY_SUCCEEDED)) */
									else
										{
											const char *status_s = GetOperationStatusAsString (status);

											PrintErrors (STM_LEVEL_WARNING, __FILE__, __LINE__, "ParseLuceneResults failed for \"%s\"", keyword_s);
										}

								}		/* if (SearchLucene (lucene_p, keyword_s, facets_p, "drill-down", page_number, page_size)) */
							else
								{
									PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "SearchLucene for \"%s\" failed", keyword_s);
								}

						}		/* if (SetLuceneToolName ("search_keywords")) */
					else
						{
							PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "SetLuceneToolName to \"search-keywords\" failed");
						}


					if (facets_p)
						{
							FreeLinkedList (facets_p);
						}

				}		/* if (success_flag) */
			else
				{
					PrintErrors (STM_LEVEL_WARNING, __FILE__, __LINE__, "Failed to allocate facets list for \"%s\"", keyword_s);
				}

			FreeLuceneTool (lucene_p);
		}		/* if (lucene_p) */
	else
		{
			PrintErrors (STM_LEVEL_WARNING, __FILE__, __LINE__, "Failed to allocate lucene tool for \"%s\"", keyword_s);
		}


	SetServiceJobStatus (job_p, status);
}


static bool AddFieldTrialResultsFromLuceneResults (const json_t *document_p, const uint32 index, void *data_p)
{
	bool success_flag = false;
	SearchData *search_data_p = (SearchData *) data_p;
	const char *id_s = GetJSONString (document_p, LUCENE_ID_S);

	if (id_s)
		{
			const char *type_s = GetJSONString (document_p, "@type");

			if (type_s)
				{
					FieldTrialDatatype datatype = GetDatatypeFromString (type_s);

					switch (datatype)
						{
							case DFTD_FIELD_TRIAL:
//								success_flag = FindAndAddResultToServiceJob (id_s, search_data_p -> sd_format, search_data_p -> sd_job_p, NULL, GetFieldTrialJSONForId, search_data_p -> sd_service_data_p);
								{
									FieldTrial *trial_p = GetFieldTrialByIdString (id_s, search_data_p -> sd_format, search_data_p -> sd_service_data_p);

									if (trial_p)
										{
											if (AddFieldTrialToServiceJob (search_data_p -> sd_job_p, trial_p, search_data_p -> sd_format, search_data_p -> sd_service_data_p))
												{
													success_flag = true;
												}
											else
												{
													PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to add field trial %s to ServiceJob", trial_p -> ft_name_s);
												}

											FreeFieldTrial (trial_p);
										}
								}
								break;

							case DFTD_STUDY:
								success_flag = FindAndAddResultToServiceJob (id_s, search_data_p -> sd_format, search_data_p -> sd_job_p, NULL, GetStudyJSONForId, DFTD_STUDY, search_data_p -> sd_service_data_p);
								break;

							case DFTD_MEASURED_VARIABLE:
								{
									MeasuredVariable *mv_p = GetMeasuredVariableByIdString (id_s, search_data_p -> sd_service_data_p);

									if (mv_p)
										{
											if (AddMeasuredVariableToServiceJob (search_data_p -> sd_job_p, mv_p, search_data_p -> sd_format, search_data_p -> sd_service_data_p))
												{
													success_flag = true;
												}
											else
												{
													PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to add MeasuredVariable %s to ServiceJob", mv_p -> mv_variable_term_p -> st_name_s);
												}

											FreeMeasuredVariable (mv_p);
										}
								}
							break;

							case DFTD_LOCATION:
								{
									Location *location_p = GetLocationByIdString (id_s, search_data_p -> sd_format, search_data_p -> sd_service_data_p);

									if (location_p)
										{
											if (AddLocationToServiceJob (search_data_p -> sd_job_p, location_p, search_data_p -> sd_format, search_data_p -> sd_service_data_p))
												{
													success_flag = true;
												}
											else
												{
													const char *name_s = "";

													if (location_p -> lo_address_p)
														{
															name_s = location_p -> lo_address_p -> ad_name_s;
														}

													PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to add Location %s to ServiceJob", name_s);
												}

											FreeLocation (location_p);
										}
								}
							break;

						case DFTD_PROGRAMME:
							{
								Programme *program_p = GetProgrammeByIdString (id_s, search_data_p -> sd_format, search_data_p -> sd_service_data_p);

								if (program_p)
									{
										if (AddProgrammeToServiceJob (search_data_p -> sd_job_p, program_p, VF_CLIENT_FULL, search_data_p -> sd_service_data_p))
											{
												success_flag = true;
											}
										else
											{
												PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to add Programme %s to ServiceJob", program_p -> pr_name_s);
											}

										FreeProgramme (program_p);
									}
							}
							break;

						case DFTD_TREATMENT:
							{
								Treatment *treatment_p = GetTreatmentByIdString (id_s, search_data_p -> sd_format, search_data_p -> sd_service_data_p);

								if (treatment_p)
									{
										if (AddTreatmentToServiceJob (search_data_p -> sd_job_p, treatment_p, VF_CLIENT_FULL, search_data_p -> sd_service_data_p))
											{
												success_flag = true;
											}
										else
											{
												PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to add Treatment %s to ServiceJob", treatment_p -> tr_ontology_term_p -> st_name_s);
											}

										FreeTreatment (treatment_p);
									}
							}
							break;


						default:
							break;

						}		/* switch (datatype) */

				}		/* if (type_s) */

		}		/* if (id_s) */
	else
		{
			PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to get \"_id\" from document");
		}

	return success_flag;
}

