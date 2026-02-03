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
 * experimental_area.c
 *
 *  Created on: 18 Sep 2018
 *      Author: billy
 */


#define ALLOCATE_STUDY_TAGS (1)
#include "study.h"
#include "memory_allocations.h"
#include "string_utils.h"
#include "plot.h"
#include "row.h"
#include "standard_row.h"
#include "treatment_factor.h"
#include "treatment_factor_jobs.h"
#include "location.h"
#include "dfw_util.h"
#include "time_util.h"
#include "crop_jobs.h"

#include "study_jobs.h"
#include "indexing.h"
#include "programme.h"
#include "person.h"
#include "phenotype_statistics.h"
#include "handbook_generator.h"
#include "person_jobs.h"
#include "mongodb_util.h"

#ifdef ENABLE_MARTI
	#include "marti_util.h"
#endif

/*
 * DB COLUMN NAMES
 */

/*
 * STATIC PROTOTYPES
 */
static void *GetStudyCallback (const json_t *json_p, const ViewFormat format, const FieldTrialServiceData *data_p);


static bool AddPlotsToJSON (Study *study_p, json_t *study_json_p, const ViewFormat format, JSONProcessor *processor_p, const FieldTrialServiceData *data_p);


static bool AddValidAspectToJSON (const Study *study_p, json_t *study_json_p);

static bool AddValidCropToJSON (Crop *crop_p, json_t *study_json_p, const char * const key_s, const ViewFormat format, const FieldTrialServiceData *data_p);


static bool AddParentFieldTrialToJSON (Study *study_p, json_t *study_json_p, const FieldTrialServiceData *data_p);

static bool AddDefaultPlotValuesToJSON (const Study *study_p, json_t *study_json_p, const FieldTrialServiceData *data_p);


static bool AddGrandParentProgramToJSON (Study *study_p, json_t *study_json_p, const ViewFormat format, const FieldTrialServiceData *data_p);

static bool AddTreatmentsToJSON (const Study *study_p, json_t *study_json_p, const ViewFormat format);

static bool AddTreatmentsFromJSON (Study *study_p, const json_t *study_json_p, const FieldTrialServiceData *data_p);


static bool AddAccessionsToJSON (Study *study_p, json_t *study_json_p, const ViewFormat format, const FieldTrialServiceData *data_p);


static bool AddStatisticsFromJSON (Study *study_p, const json_t *study_json_p, const FieldTrialServiceData *data_p);



static bool AddCommonStudyJSONValues (Study *study_p, json_t *study_json_p, const ViewFormat format, const FieldTrialServiceData *data_p);

static bool AddFrictionlessDataLink (const Study * const study_p, json_t *study_json_p, const FieldTrialServiceData *data_p);

static bool AddHandbookLinks (const Study * const study_p, json_t *study_json_p, const FieldTrialServiceData *data_p);


static bool SetDateFromStudyJSON (const json_t *json_p, const char *key_s, uint32 *year_p, const char *deprecated_key_s);

static void DetachStudyFromParent (Study *study_p);

static bool AddCommonPlotValuesToJSON (Study *study_p, json_t *study_json_p, const ViewFormat format, const FieldTrialServiceData *data_p);

static bool AddPersonFromJSON (Person *person_p, void *user_data_p, MEM_FLAG *mem_p);



/*
 * API FUNCTIONS
 */



Study *AllocateStudy (bson_oid_t *id_p,  Metadata *metadata_p,  const char *name_s, const char *data_url_s, const char *aspect_s, const char *slope_s,
											struct Location *location_p, FieldTrial *parent_field_trial_p,
											MEM_FLAG parent_field_trial_mem, Crop *current_crop_p, Crop *previous_crop_p, const char *description_s,
											const char *design_s, const char *growing_conditions_s, const char *phenotype_gathering_notes_s,
											const uint32 *num_rows_p, const uint32 *num_cols_p, const uint32 *num_replicates_p, const double64 *plot_width_p, const double64 *plot_length_p,
											const char *weather_s, const json_t *shape_p, const double64 *plot_horizontal_gap_p, const double64 *plot_vertical_gap_p,
											const uint32 *plot_rows_per_block_p, const uint32 *plot_columns_per_block_p, const double64 *plot_block_horizontal_gap_p,
											const double64 *plot_block_vertical_gap_p,
											Person *curator_p, Person *contact_p,
											const uint32 *sowing_year_p, const uint32 *harvest_year_p,
											const char *plan_changes_s, const char *physical_samples_collected_s, const char *data_not_included_s,
											const char *photo_url_s, const char *image_collection_notes_s,
											const char *gps_notes_s,
											const FieldTrialServiceData *data_p)
{
	char *copied_name_s = EasyCopyToNewString (name_s);

	if (copied_name_s)
		{
			char *copied_url_s = NULL;

			if (CloneValidString (data_url_s, &copied_url_s))
				{
					char *copied_aspect_s = NULL;

					if (CloneValidString (aspect_s, &copied_aspect_s))
						{
							char *copied_slope_s = NULL;

							if (CloneValidString (slope_s, &copied_slope_s))
								{
									LinkedList *plots_p = AllocateLinkedList (FreePlotNode);

									if (plots_p)
										{
											LinkedList *stats_p = AllocateLinkedList (FreePhenotypeStatisticsNode);

											if (stats_p)
												{
													char *copied_description_s = NULL;

													if (CloneValidString (description_s, &copied_description_s))
														{
															char *copied_design_s = NULL;

															if (CloneValidString (design_s, &copied_design_s))
																{
																	char *copied_growing_conditions_s = NULL;

																	if (CloneValidString (growing_conditions_s, &copied_growing_conditions_s))
																		{
																			char *copied_phenotype_notes_s = NULL;

																			if (CloneValidString (phenotype_gathering_notes_s, &copied_phenotype_notes_s))
																				{
																					char *copied_weather_s = NULL;

																					if (CloneValidString (weather_s, &copied_weather_s))
																						{
																							char *copied_plan_changes_s = NULL;

																							if (CloneValidString (plan_changes_s, &copied_plan_changes_s))
																								{
																									char *copied_physical_samples_collected_s = NULL;

																									if (CloneValidString (physical_samples_collected_s, &copied_physical_samples_collected_s))
																										{
																											char *copied_data_not_included_s = NULL;

																											if (CloneValidString (data_not_included_s, &copied_data_not_included_s))
																												{
																													double64 *copied_plot_width_p = NULL;

																													if (CopyValidReal (plot_width_p, &copied_plot_width_p))
																														{
																															double64 *copied_plot_length_p = NULL;

																															if (CopyValidReal (plot_length_p, &copied_plot_length_p))
																																{
																																	uint32 *copied_num_rows_p = NULL;

																																	if (CopyValidUnsignedInteger (num_rows_p, &copied_num_rows_p))
																																		{
																																			uint32 *copied_num_cols_p = NULL;

																																			if (CopyValidUnsignedInteger (num_cols_p, &copied_num_cols_p))
																																				{
																																					uint32 *copied_num_replicates_p = NULL;

																																					if (CopyValidUnsignedInteger (num_replicates_p, &copied_num_replicates_p))
																																						{
																																							json_t *copied_shape_p = NULL;

																																							if ((shape_p == NULL) || (copied_shape_p = json_deep_copy (shape_p)))
																																								{
																																									double64 *copied_plot_hgap_p = NULL;

																																									if (CopyValidReal (plot_horizontal_gap_p, &copied_plot_hgap_p))
																																										{
																																											double64 *copied_plot_vgap_p = NULL;

																																											if (CopyValidReal (plot_vertical_gap_p, &copied_plot_vgap_p))
																																												{
																																													uint32 *copied_plot_rows_per_block_p = NULL;

																																													if (CopyValidUnsignedInteger (plot_rows_per_block_p, &copied_plot_rows_per_block_p))
																																														{
																																															uint32 *copied_plot_columns_per_block_p = NULL;

																																															if (CopyValidUnsignedInteger (plot_columns_per_block_p, &copied_plot_columns_per_block_p))
																																																{
																																																	double64 *copied_plot_block_horizontal_gap_p = NULL;

																																																	if (CopyValidReal (plot_block_horizontal_gap_p, &copied_plot_block_horizontal_gap_p))
																																																		{
																																																			double64 *copied_plot_block_vertical_gap_p = NULL;

																																																			if (CopyValidReal (plot_block_vertical_gap_p, &copied_plot_block_vertical_gap_p))
																																																				{
																																																					LinkedList *treatments_p = AllocateLinkedList (FreeTreatmentFactorNode);

																																																					if (treatments_p)
																																																						{
																																																							uint32 *copied_sowing_year_p = NULL;

																																																							if (CopyValidUnsignedInteger (sowing_year_p, &copied_sowing_year_p))
																																																								{
																																																									uint32 *copied_harvest_year_p = NULL;

																																																									if (CopyValidUnsignedInteger (harvest_year_p, &copied_harvest_year_p))
																																																										{
																																																											char *copied_photo_url_s = NULL;

																																																											if (CloneValidString (photo_url_s, &copied_photo_url_s))
																																																												{
																																																													char *copied_image_collection_notes_s = NULL;

																																																													if (CloneValidString (image_collection_notes_s, &copied_image_collection_notes_s))
																																																														{
																																																															char *copied_gps_notes_s = NULL;

																																																															if (CloneValidString (gps_notes_s, &copied_gps_notes_s))
																																																																{
																																																																	LinkedList *contributors_p = AllocateLinkedList (FreePersonNode);

																																																																	if (contributors_p)
																																																																		{
																																																																			Study *study_p = (Study *) AllocMemory (sizeof (Study));

																																																																			if (study_p)
																																																																				{
																																																																					study_p -> st_id_p = id_p;
																																																																					study_p -> st_metadata_p = metadata_p;
																																																																					study_p -> st_name_s = copied_name_s;
																																																																					study_p -> st_data_url_s = copied_url_s;
																																																																					study_p -> st_aspect_s = copied_aspect_s;
																																																																					study_p -> st_slope_s = copied_slope_s;
																																																																					study_p -> st_parent_p = parent_field_trial_p;
																																																																					study_p -> st_parent_field_trial_mem = parent_field_trial_mem;
																																																																					study_p -> st_location_p = location_p;
																																																																					study_p -> st_plots_p = plots_p;
																																																																					study_p -> st_current_crop_p = current_crop_p;
																																																																					study_p -> st_previous_crop_p = previous_crop_p;
																																																																					study_p -> st_description_s = copied_description_s;
																																																																					study_p -> st_growing_conditions_s = copied_growing_conditions_s;
																																																																					study_p -> st_phenotype_gathering_notes_s = copied_phenotype_notes_s;
																																																																					study_p -> st_design_s = copied_design_s;

																																																																					study_p -> st_default_plot_width_p = copied_plot_width_p;
																																																																					study_p -> st_default_plot_length_p = copied_plot_length_p;
																																																																					study_p -> st_num_rows_p = copied_num_rows_p;
																																																																					study_p -> st_num_columns_p = copied_num_cols_p;
																																																																					study_p -> st_num_replicates_p = copied_num_replicates_p;

																																																																					study_p -> st_weather_link_s = copied_weather_s;
																																																																					study_p -> st_shape_p = copied_shape_p;

																																																																					study_p -> st_plot_horizontal_gap_p = copied_plot_hgap_p;
																																																																					study_p -> st_plot_vertical_gap_p = copied_plot_vgap_p;
																																																																					study_p -> st_plots_rows_per_block_p = copied_plot_rows_per_block_p;
																																																																					study_p -> st_plots_columns_per_block_p = copied_plot_columns_per_block_p;
																																																																					study_p -> st_plot_block_horizontal_gap_p = copied_plot_block_horizontal_gap_p;
																																																																					study_p -> st_plot_block_vertical_gap_p = copied_plot_block_vertical_gap_p;

																																																																					study_p -> st_treatments_p = treatments_p;

																																																																					study_p -> st_curator_p = curator_p;
																																																																					study_p -> st_contact_p = contact_p;

																																																																					study_p -> st_predicted_sowing_year_p = copied_sowing_year_p;
																																																																					study_p -> st_predicted_harvest_year_p = copied_harvest_year_p;


																																																																					study_p -> st_plan_changes_s = copied_plan_changes_s;
																																																																					study_p -> st_physical_samples_collected_s = copied_physical_samples_collected_s;
																																																																					study_p -> st_data_not_included_s= copied_data_not_included_s;

																																																																					study_p -> st_photo_url_s = copied_photo_url_s;
																																																																					study_p -> st_image_collection_notes_s = copied_image_collection_notes_s;

																																																																					study_p -> st_shape_notes_s = copied_gps_notes_s;
																																																																					study_p -> st_phenotypes_p = stats_p;

																																																																					study_p -> st_contributors_p = contributors_p;
																																																																					study_p -> st_accessions_p = NULL;

																																																																					return study_p;
																																																																				}

																																																																			FreeLinkedList (contributors_p);
																																																																		}		/* if (contributors_p) */
																																																																	else
																																																																		{
																																																																			PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to allocate Contributors list");

																																																																		}



																																																																	if (copied_gps_notes_s)
																																																																		{
																																																																			FreeCopiedString (copied_gps_notes_s);
																																																																		}

																																																																}		/* if (CloneValidString (gps_notes_s, &copied_gps_notes_s)) */


																																																															if (copied_image_collection_notes_s)
																																																																{
																																																																	FreeCopiedString (copied_image_collection_notes_s);
																																																																}

																																																														}

																																																													if (copied_photo_url_s)
																																																														{
																																																															FreeCopiedString (copied_photo_url_s);
																																																														}

																																																												}		/* if (CloneValidString (photo_url_s, &copied_photo_url_s)) */



																																																											if (copied_harvest_year_p)
																																																												{
																																																													FreeMemory (copied_harvest_year_p);
																																																												}

																																																										}

																																																									if (copied_sowing_year_p)
																																																										{
																																																											FreeMemory (copied_sowing_year_p);
																																																										}

																																																								}

																																																							FreeLinkedList (treatments_p);
																																																						}		/* if (treatments_p) */


																																																						if (copied_plot_block_vertical_gap_p)
																																																							{
																																																								FreeMemory (copied_plot_block_vertical_gap_p);
																																																							}

																																																				}		/* if (CopyValidReal (plot_block_vertical_gap_p, &copied_plot_block_vertical_gap_p)) */

																																																			if (copied_plot_block_horizontal_gap_p)
																																																				{
																																																					FreeMemory (copied_plot_block_horizontal_gap_p);
																																																				}

																																																		}		/* if (CopyValidReal (plot_block_horizontal_gap_p, &plot_block_horizontal_gap_p)) */

																																																	if (copied_plot_columns_per_block_p)
																																																		{
																																																			FreeMemory (copied_plot_columns_per_block_p);
																																																		}

																																																}		/* if (CopyValidUnsignedInteger (plots_per_vertical_block_p, &copied_plot_columns_per_block_p)) */

																																															if (copied_plot_rows_per_block_p)
																																																{
																																																	FreeMemory (copied_plot_rows_per_block_p);
																																																}

																																														}		/* if (CopyValidUnsignedInteger (plots_per_horizontal_block_p, &copied_plot_rows_per_block_p)) */


																																													if (copied_plot_vgap_p)
																																														{
																																															FreeMemory (copied_plot_vgap_p);
																																														}

																																												}		/* if (CopyValidReal (plot_vertical_gap_p, &copied_plot_vgap_p)) */

																																											if (copied_plot_hgap_p)
																																												{
																																													FreeMemory (copied_plot_hgap_p);
																																												}

																																										}		/* if (CopyValidReal (plot_horizontal_gap_p, &copied_plot_hgap_p)) */


																																									if (copied_shape_p)
																																										{
																																											json_decref (copied_shape_p);
																																										}
																																								}		/* if ((shape_p == NULL) || (copied_shape_p = json_deep_copy (shape_p))) */

																																							if (copied_num_replicates_p)
																																								{
																																									FreeMemory (copied_num_replicates_p);
																																								}

																																						}		/* if (CopyValidUnsignedInteger (num_cols_p, &copied_num_replicates_p)) */


																																					if (copied_num_cols_p)
																																						{
																																							FreeMemory (copied_num_cols_p);
																																						}
																																				}

																																			if (copied_num_rows_p)
																																				{
																																					FreeMemory (copied_num_rows_p);
																																				}

																																		}

																																	if (copied_plot_length_p)
																																		{
																																			FreeMemory (copied_plot_length_p);
																																		}

																																}


																															if (copied_plot_width_p)
																																{
																																	FreeMemory (copied_plot_width_p);
																																}
																														}



																													if (copied_data_not_included_s)
																														{
																															FreeCopiedString (copied_data_not_included_s);
																														}

																												}		/* if (CloneValidString (data_not_included_s, &copied_data_not_included_s)) */

																											if (copied_physical_samples_collected_s)
																												{
																													FreeCopiedString (copied_physical_samples_collected_s);
																												}

																										}		/* if (CloneValidString (physical_samples_collected_s, &copied_physical_samples_collected_s)) */

																									if (copied_plan_changes_s)
																										{
																											FreeCopiedString (copied_plan_changes_s);
																										}

																								}		/* if (CloneValidString (plan_changes_s, &copied_plan_changes_s)) */


																							if (copied_weather_s)
																								{
																									FreeCopiedString (copied_weather_s);
																								}

																						}		/* if (CloneValidString (weather_s, &copied_weather_s)) */


																					if (copied_phenotype_notes_s)
																						{
																							FreeCopiedString (copied_phenotype_notes_s);
																						}

																				}		/* if (CloneValidString (phenotype_gathering_notes_s, &copied_phenotype_notes_s)) */

																			if (copied_growing_conditions_s)
																				{
																					FreeCopiedString (copied_growing_conditions_s);
																				}

																		}		/* if (CloneValidString (growing_conditions_s, &copied_growing_conditions_s)) */

																	if (copied_design_s)
																		{
																			FreeCopiedString (copied_design_s);
																		}

																}		/* if (CloneValidString (design_s, &copied_design_s)) */


															if (copied_description_s)
																{
																	FreeCopiedString (copied_description_s);
																}

														}		/* if (CloneValidString (notes_s, &copied_notes_s)) */


													FreeLinkedList (stats_p);
												}		/* if (stats_p) */
											else
												{
													PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__ , "Failed to allocate stats list for study");
												}

											FreeLinkedList (plots_p);
										}		/* if (plots_p) */

									if (copied_slope_s)
										{
											FreeCopiedString (copied_slope_s);
										}

								}		/* if (CloneValidString (slope_s, &copied_slope_s)) */

							if (copied_aspect_s)
								{
									FreeCopiedString (copied_aspect_s);
								}

						}		/* if (CloneValidString (aspect_s, &copied_aspect_s)) */

					if (copied_url_s)
						{
							FreeCopiedString (copied_url_s);
						}

				}		/* if (CloneValidString (data_url_s, &copied_url_s)) */

			FreeCopiedString (copied_name_s);
		}		/* if (copied_name_s) */
	else
		{

		}

	return NULL;
}


void FreeStudy (Study *study_p)
{
	if (study_p -> st_id_p)
		{
			FreeBSONOid (study_p -> st_id_p);
		}

	if (study_p -> st_description_s)
		{
			FreeCopiedString (study_p -> st_description_s);
		}

	if (study_p -> st_data_url_s)
		{
			FreeCopiedString (study_p -> st_data_url_s);
		}


	if (study_p -> st_aspect_s)
		{
			FreeCopiedString (study_p -> st_aspect_s);
		}

	if (study_p -> st_slope_s)
		{
			FreeCopiedString (study_p -> st_slope_s);
		}

	if (study_p -> st_location_p)
		{
			FreeLocation (study_p -> st_location_p);
		}

	if (study_p -> st_plots_p)
		{
			FreeLinkedList (study_p -> st_plots_p);
		}

	if (study_p -> st_current_crop_p)
		{
			FreeCrop (study_p -> st_current_crop_p);
		}

	if (study_p -> st_previous_crop_p)
		{
			FreeCrop (study_p -> st_previous_crop_p);
		}


	if (study_p -> st_phenotype_gathering_notes_s)
		{
			FreeCopiedString (study_p -> st_phenotype_gathering_notes_s);
		}

	if (study_p -> st_design_s)
		{
			FreeCopiedString (study_p -> st_design_s);
		}

	if (study_p -> st_growing_conditions_s)
		{
			FreeCopiedString (study_p -> st_growing_conditions_s);
		}


	if (study_p -> st_physical_samples_collected_s)
		{
			FreeCopiedString (study_p -> st_physical_samples_collected_s);
		}

	if (study_p -> st_data_not_included_s)
		{
			FreeCopiedString (study_p -> st_data_not_included_s);
		}

	if (study_p -> st_plan_changes_s)
		{
			FreeCopiedString (study_p -> st_plan_changes_s);
		}

	if (study_p -> st_default_plot_width_p)
		{
			FreeMemory (study_p -> st_default_plot_width_p);
		}

	if (study_p -> st_default_plot_length_p)
		{
			FreeMemory (study_p -> st_default_plot_length_p);
		}

	if (study_p -> st_num_rows_p)
		{
			FreeMemory (study_p -> st_num_rows_p);
		}

	if (study_p -> st_num_columns_p)
		{
			FreeMemory (study_p -> st_num_columns_p);
		}


	if (study_p -> st_num_replicates_p)
		{
			FreeMemory (study_p -> st_num_replicates_p);
		}


	if (study_p -> st_parent_p)
		{
			DetachStudyFromParent (study_p);
		}

	if (study_p -> st_weather_link_s)
		{
			FreeCopiedString (study_p -> st_weather_link_s);
		}

	if (study_p -> st_shape_p)
		{
			json_decref (study_p -> st_shape_p);
		}

	if (study_p -> st_plot_horizontal_gap_p)
		{
			FreeMemory (study_p -> st_plot_horizontal_gap_p);
		}

	if (study_p -> st_plot_vertical_gap_p)
		{
			FreeMemory (study_p -> st_plot_vertical_gap_p);
		}

	if (study_p -> st_plots_rows_per_block_p)
		{
			FreeMemory (study_p -> st_plots_rows_per_block_p);
		}

	if (study_p -> st_plots_columns_per_block_p)
		{
			FreeMemory (study_p -> st_plots_columns_per_block_p);
		}

	if (study_p -> st_plot_block_horizontal_gap_p)
		{
			FreeMemory (study_p -> st_plot_block_horizontal_gap_p);
		}

	if (study_p -> st_plot_block_vertical_gap_p)
		{
			FreeMemory (study_p -> st_plot_block_vertical_gap_p);
		}

	if (study_p -> st_treatments_p)
		{
			FreeLinkedList (study_p -> st_treatments_p);
		}


	if (study_p -> st_curator_p)
		{
			FreePerson (study_p -> st_curator_p);
		}

	if (study_p -> st_contact_p)
		{
			FreePerson (study_p -> st_contact_p);
		}

	if (study_p -> st_predicted_sowing_year_p)
		{
			FreeMemory (study_p -> st_predicted_sowing_year_p);
		}


	if (study_p -> st_predicted_harvest_year_p)
		{
			FreeMemory (study_p -> st_predicted_harvest_year_p);
		}


	if (study_p -> st_phenotypes_p)
		{
			FreeLinkedList (study_p -> st_phenotypes_p);
		}


	if (study_p -> st_photo_url_s)
		{
			FreeCopiedString (study_p -> st_photo_url_s);
		}

	if (study_p -> st_image_collection_notes_s)
		{
			FreeCopiedString (study_p -> st_image_collection_notes_s);
		}


	if (study_p -> st_shape_notes_s)
		{
			FreeCopiedString (study_p -> st_shape_notes_s);
		}

	if (study_p -> st_name_s)
		{
			FreeCopiedString (study_p -> st_name_s);
		}

	if (study_p -> st_contributors_p)
		{
			FreeLinkedList (study_p -> st_contributors_p);
		}

	if (study_p -> st_accessions_p)
		{
			json_decref (study_p -> st_accessions_p);
		}

	FreeMemory (study_p);
}


static void DetachStudyFromParent (Study *study_p)
{
	if ((study_p -> st_parent_field_trial_mem == MF_DEEP_COPY) || (study_p -> st_parent_field_trial_mem == MF_SHALLOW_COPY))
		{
			RemoveFieldTrialStudy (study_p -> st_parent_p, study_p);

			if (GetNumberOfFieldTrialStudies (study_p -> st_parent_p) == 0)
				{
					FreeFieldTrial (study_p -> st_parent_p);
				}
		}

}


StudyNode *AllocateStudyNode (Study *study_p)
{
	StudyNode *st_node_p = (StudyNode *) AllocMemory (sizeof (StudyNode));

	if (st_node_p)
		{
			InitListItem (& (st_node_p -> stn_node));

			st_node_p -> stn_study_p = study_p;
		}

	return st_node_p;
}

void FreeStudyNode (ListItem *node_p)
{
	StudyNode *st_node_p = (StudyNode *) node_p;

	if (st_node_p -> stn_study_p)
		{
			FreeStudy (st_node_p -> stn_study_p);
		}

	FreeMemory (st_node_p);
}


bool AddStudyPlotsJSONDirectly (Study *study_p, json_t *study_json_p,  const FieldTrialServiceData *data_p)
{
	bool success_flag = false;

	return success_flag;
}


bool GetStudyPlots (Study *study_p, const ViewFormat format, FieldTrialServiceData *data_p)
{
	bool success_flag = false;

	ClearLinkedList (study_p -> st_plots_p);

	if (SetMongoToolCollection (data_p -> dftsd_mongo_p, data_p -> dftsd_collection_ss [DFTD_PLOT]))
		{
			bson_t *query_p = BCON_NEW (PL_PARENT_STUDY_S, BCON_OID (study_p -> st_id_p));

			/*
			 * Make the query to get the matching plots
			 */
			if (query_p)
				{
					bson_t *opts_p =  BCON_NEW ( "sort", "{", PL_ROW_INDEX_S, BCON_INT32 (1), PL_COLUMN_INDEX_S, BCON_INT32 (1), "}");

					if (opts_p)
						{
							json_t *results_p = GetAllMongoResultsAsJSON (data_p -> dftsd_mongo_p, query_p, opts_p);

							if (results_p)
								{
									if (json_is_array (results_p))
										{
											size_t i;
											const size_t num_results = json_array_size (results_p);

											success_flag = true;

											if (num_results > 0)
												{
													json_t *plot_json_p;

													json_array_foreach (results_p, i, plot_json_p)
														{
															Plot *plot_p = GetPlotFromJSON (plot_json_p, study_p, format, data_p);

															if (plot_p)
																{
																	if (! (AddPlotToStudy (study_p, plot_p)))
																		{
																			PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, plot_json_p, "Failed to add plot to study's list");
																			FreePlot (plot_p);
																		}
																}

														}		/* json_array_foreach (results_p, i, entry_p) */

												}		/* if (num_results > 0) */


										}		/* if (json_is_array (results_p)) */

									json_decref (results_p);
								}		/* if (results_p) */

							bson_destroy (opts_p);
						}		/* if (opts_p) */

					bson_destroy (query_p);
				}		/* if (query_p) */

		}

	return success_flag;
}



bool AddPlotToStudy (Study *study_p, Plot *plot_p)
{
	bool success_flag = false;
	PlotNode *node_p = AllocatePlotNode (plot_p);

	if (node_p)
		{
			LinkedListAddTail (study_p -> st_plots_p, & (node_p -> pn_node));
			success_flag = true;
		}
	else
		{
			PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to add plot to study \"%s\"", study_p -> st_name_s);
		}

	return success_flag;
}



OperationStatus SaveStudy (Study *study_p, ServiceJob *job_p, FieldTrialServiceData *data_p, const char *url_key_s)
{
	OperationStatus status = OS_FAILED;
	bson_t *selector_p = NULL;
	bool success_flag = PrepareSaveData (& (study_p -> st_id_p), &selector_p);

	if (success_flag)
		{
			json_t *study_json_p = GetStudyAsJSON (study_p, VF_STORAGE, NULL, data_p);

			if (study_json_p)
				{
					if (SaveAndBackupMongoDataWithTimestamp (data_p -> dftsd_mongo_p, study_json_p, data_p -> dftsd_collection_ss [DFTD_STUDY], 
							data_p -> dftsd_backup_collection_ss [DFTD_STUDY], DFT_BACKUPS_ID_KEY_S, selector_p, MONGO_TIMESTAMP_S))
						{
							char *id_s = GetBSONOidAsString (study_p -> st_id_p);

							if (id_s)
								{
									json_t *info_p = NULL;

									status = OS_SUCCEEDED;

									if (data_p -> dftsd_study_cache_path_s)
										{
											ClearCachedStudy (id_s, data_p);
										}

									if (data_p -> dftsd_assets_path_s)
										{
											if (!SaveStudyAsFrictionlessData (study_p, data_p))
												{
													PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "SaveStudyAsFrictionlessData () failed for Study \"%s\"", study_p -> st_name_s);
												}
										}

									/*
									 * If we have the front-end web address to view the study,
									 * save it to the ServiceJob.
									 */
									if (!url_key_s)
										{
											url_key_s = data_p -> dftsd_view_study_url_s;
										}

									if (url_key_s)
										{
											SetFieldTrialServiceJobURL (job_p, url_key_s, id_s);
										}

									info_p = json_object ();

									if (info_p)
										{
											bool added_flag = false;

											if (AddContext (info_p))
												{
													json_t *dest_record_p = GetDataResourceAsJSONByParts (PROTOCOL_INLINE_S, NULL, id_s, info_p);

													if (dest_record_p)
														{
															added_flag = true;

															AddImage (dest_record_p, DFTD_STUDY, data_p);

															if (AddResultToServiceJob (job_p, dest_record_p))
																{
																	success_flag = true;
																}
															else
																{
																	json_decref (dest_record_p);
																}

														}		/* if (dest_record_p) */

													if (!added_flag)
														{
															json_decref (info_p);
														}
												}		/* if (AddContext (trial_json_p)) */


										}


									FreeBSONOidString (id_s);
								}
							else
								{
									status = OS_PARTIALLY_SUCCEEDED;

									PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to clear potential cached Study \"%s\"", study_p -> st_name_s);
								}

						}
					else
						{
							PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, study_json_p, "SaveAndBackupMongoDataWithTimestamp () failed for Study \"%s\"", study_p -> st_name_s);
						}

					json_decref (study_json_p);
				}		/* if (study_json_p) */
			else
				{
					PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "GetStudyAsJSON () failed for Study \"%s\"", study_p -> st_name_s);
				}

			/*
			 * Index the Study using Lucene
			 */
			if ((status == OS_SUCCEEDED) || (status == OS_PARTIALLY_SUCCEEDED))
				{
					OperationStatus s = IndexStudy (study_p, job_p, NULL, data_p);

					if (s != OS_SUCCEEDED)
						{
							status = OS_PARTIALLY_SUCCEEDED;
							AddGeneralErrorMessageToServiceJob (job_p, "Saved study but failed to index for searching");
							PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "IndexStudy () failed for Study \"%s\"", study_p -> st_name_s);
						}

					s = GenerateStudyAsPDF (study_p, data_p);
					if (s != OS_SUCCEEDED)
						{
							status = OS_PARTIALLY_SUCCEEDED;
							AddGeneralErrorMessageToServiceJob (job_p, "Saved study but failed to generate handbook");
							PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "GenerateStudyAsPDF () failed for Study \"%s\" with status %d", study_p -> st_name_s, s);
						}

				}

		}		/* if (success_flag) */

	SetServiceJobStatus (job_p, status);

	return status;
}


OperationStatus IndexStudy (Study *study_p, ServiceJob *job_p, const char *job_name_s, FieldTrialServiceData *data_p)
{
	OperationStatus status = OS_FAILED;
	const ViewFormat format = VF_INDEXING;

	if (GetStudyPlots (study_p, format, data_p))
		{
			json_t *study_json_p = GetStudyAsJSON (study_p, format, NULL, data_p);

			if (study_json_p)
				{
					status = IndexData (job_p, study_json_p, job_name_s);

					if (status != OS_SUCCEEDED)
						{
							PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, study_json_p, "Failed to index Study \"%s\" as JSON to Lucene", study_p -> st_name_s);
							AddGeneralErrorMessageToServiceJob (job_p, "Study saved but failed to index for searching");
						}

					json_decref (study_json_p);
				}
			else
				{
					PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "GetStudyAsJSON for \"%s\" failed", study_p -> st_name_s);
					AddGeneralErrorMessageToServiceJob (job_p, "Study saved but failed to index for searching");
				}
		}

	return status;
}


bool AddStudyContributor (Study *study_p, Person *person_p, MEM_FLAG mf)
{
	bool success_flag = false;
	PersonNode *node_p = AllocatePersonNode (person_p);

	if (node_p)
		{
			LinkedListAddTail (study_p -> st_contributors_p, & (node_p -> pn_node));
			success_flag  = true;
		}

	return success_flag;
}



json_t *GetStudyAsJSON (Study *study_p, const ViewFormat format, JSONProcessor *processor_p, FieldTrialServiceData *data_p)
{
	json_t *study_json_p = json_object ();

	if (study_json_p)
		{
			if (AddCommonStudyJSONValues (study_p, study_json_p, format, data_p))
				{
					if (AddMetadataToJSON (study_p -> st_metadata_p, study_json_p, format))
						{
							bool add_item_flag = false;

							/*
							 * Add the location
							 */
							if ((format == VF_CLIENT_FULL) || (format == VF_CLIENT_MINIMAL) || (format == VF_INDEXING))
								{
									json_t *location_json_p = GetLocationAsJSON (study_p -> st_location_p);

									if (location_json_p)
										{
											if (json_object_set_new (study_json_p, ST_LOCATION_S, location_json_p) == 0)
												{
													if (AddValidCropToJSON (study_p -> st_current_crop_p, study_json_p, ST_CURRENT_CROP_S, format, data_p))
														{
															if (AddValidCropToJSON (study_p -> st_previous_crop_p, study_json_p, ST_PREVIOUS_CROP_S, format, data_p))
																{
																	if (AddParentFieldTrialToJSON (study_p, study_json_p, data_p))
																		{
																			if (AddGrandParentProgramToJSON (study_p, study_json_p, format, data_p))
																				{
																					AddFrictionlessDataLink (study_p, study_json_p, data_p);

																					AddHandbookLinks (study_p, study_json_p, data_p);

																					#ifdef ENABLE_MARTI
																					AddMartiResults (study_p, study_json_p, format, data_p);
																					#endif

																					add_item_flag = true;
																				}
																			else
																				{
																					PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, study_json_p, "Failed to add grandparent program to study \"%s\"", study_p -> st_parent_p -> ft_name_s);
																				}
																		}
																	else
																		{
																			PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, study_json_p, "Failed to add parent field trial to study \"%s\"", study_p -> st_parent_p -> ft_name_s);
																		}
																}
															else
																{
																	PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, study_json_p, "Failed to add previous crop to study \"%s\"", study_p -> st_previous_crop_p -> cr_name_s);
																}
														}
													else
														{
															PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, study_json_p, "Failed to add current crop to study \"%s\"", study_p -> st_current_crop_p -> cr_name_s);
														}

												}		/* if (json_object_set_new (study_json_p, ST_LOCATION_S, location_json_p) == 0) */
											else
												{
													json_decref (location_json_p);
													PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, study_json_p, "Failed to add location to study \"%s\"", study_p -> st_location_p -> lo_address_p -> ad_name_s);
												}
										}		/* if (location_json_p) */
									else
										{
											PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to get location \"%s\" as JSON", study_p -> st_location_p -> lo_address_p -> ad_name_s);
										}
								}
							else
								{

									if (AddNamedCompoundIdToJSON (study_json_p, study_p -> st_parent_p -> ft_id_p, ST_PARENT_FIELD_TRIAL_S))
										{

											if ((! (study_p -> st_location_p)) || (AddNamedCompoundIdToJSON (study_json_p, study_p -> st_location_p -> lo_id_p, ST_LOCATION_ID_S)))
												{
													if (AddValidCropToJSON (study_p -> st_current_crop_p, study_json_p, ST_CURRENT_CROP_S, VF_STORAGE, data_p))
														{
															if (AddValidCropToJSON (study_p -> st_previous_crop_p, study_json_p, ST_PREVIOUS_CROP_S, VF_STORAGE, data_p))
																{
																	add_item_flag = true;
																}		/* if (AddNamedCompoundIdToJSON (study_json_p, study_p -> st_previous_crop_p -> cr_id_p, ST_PREVIOUS_CROP_S)) */
															else
																{
																	PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, study_json_p, "Failed to add previous crop \"%s\"", study_p -> st_previous_crop_p -> cr_name_s);
																}
														}		/* if (AddNamedCompoundIdToJSON (study_json_p, study_p -> st_current_crop_p -> cr_id_p, ST_CURRENT_CROP_S)) */
													else
														{
															PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, study_json_p, "Failed to add current crop \"%s\"", study_p -> st_current_crop_p -> cr_name_s);
														}
												}		/* if (AddNamedCompoundIdToJSON (study_json_p, study_p -> st_location_p -> lo_id_p, ST_LOCATION_ID_S)) */
											else
												{
													PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, study_json_p, "Failed to add location \"%s\"", study_p -> st_location_p -> lo_address_p -> ad_name_s);
												}
										}		/* if (AddNamedCompoundIdToJSON (study_json_p, study_p -> st_parent_p -> ft_id_p, ST_PARENT_FIELD_TRIAL_S)) */
									else
										{
											PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, study_json_p, "Failed to add \"%s\" \"%s\"", ST_PARENT_FIELD_TRIAL_S, study_p -> st_parent_p -> ft_name_s);
										}

								}

							if (add_item_flag)
								{
									if (AddCompoundIdToJSON (study_json_p, study_p -> st_id_p))
										{
											bool success_flag = false;

											switch (format)
											{
												case VF_INDEXING:
													success_flag = true;
													break;

												case VF_CLIENT_FULL:
													{
														if (GetStudyPlots (study_p, format, data_p))
															{
																if (AddPlotsToJSON (study_p, study_json_p, format, processor_p, data_p))
																	{
																		if (study_p -> st_shape_p)
																			{
																				if (json_object_set (study_json_p, ST_SHAPE_S, study_p -> st_shape_p) == 0)
																					{
																						success_flag = true;
																					}		/*  if (json_object_set (study_json_p, ST_SHAPE_S, study_p -> st_shape_p) == 0) */
																				else
																					{
																						PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, study_p -> st_shape_p, "Failed to add shape sata to study json for \"%s\"",
																															 study_p -> st_name_s ? study_p -> st_name_s : "unknown study");
																					}

																			}
																		else
																			{
																				success_flag = true;
																			}		/* if ((!study_p -> st_shape_p) */
																	}
															}
													}
													break;

												case VF_CLIENT_MINIMAL:
													{
														json_int_t num_plots = GetNumberOfPlotsInStudy (study_p, data_p);

														if (SetJSONInteger (study_json_p, ST_NUMBER_OF_PLOTS_S, num_plots))
															{
																const bool has_shape_data_flag = (study_p -> st_shape_p) ? true : false;

																if (SetJSONBoolean (study_json_p, ST_HAS_SHAPE_S, has_shape_data_flag))
																	{
																		success_flag = true;
																	}
															}

													}
													break;

												case VF_STORAGE:
													{
														json_int_t num_plots = GetNumberOfPlotsInStudy (study_p, data_p);

														if (SetJSONInteger (study_json_p, ST_NUMBER_OF_PLOTS_S, num_plots))
															{

																if (study_p -> st_shape_p)
																	{
																		if (json_object_set (study_json_p, ST_SHAPE_S, study_p -> st_shape_p) == 0)
																			{
																				success_flag = true;
																			}		/*  if (json_object_set (study_json_p, ST_SHAPE_S, study_p -> st_shape_p) == 0) */
																		else
																			{
																				PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, study_p -> st_shape_p, "Failed to add shape sata to study json for \"%s\"",
																													 study_p -> st_name_s ? study_p -> st_name_s : "unknown study");
																			}

																	}
																else
																	{
																		success_flag = true;
																	}		/* if ((!study_p -> st_shape_p) */
															}
													}
													break;

												default:
													break;
											}



											if (success_flag)
												{
													if (AddDatatype (study_json_p, DFTD_STUDY))
														{
															return study_json_p;
														}
												}

										}		/* if (add_item_flag) */

								}		/* if (add_item_flag) */


						}		/* if (AddMetadataToJSON (study_p -> st_metadata_p, study_json_p, format)) */
					else
						{
							PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, study_json_p, "AddPermissionsGroupToJSON () failed");
						}

				}		/* if (AddCommonStudyJSONValues (study_p, study_json_p, format, data_p)) */

		}		/* if (study_json_p) */

	return NULL;
}


Study *GetStudyWithParentTrialFromJSON (const json_t *json_p, FieldTrial *parent_trial_p, const ViewFormat format, const FieldTrialServiceData *data_p)
{
	const char *name_s = GetJSONString (json_p, ST_NAME_S);
	Study *study_p = NULL;

	if (name_s)
		{
			bson_oid_t *location_id_p = GetNewUnitialisedBSONOid ();

			if (location_id_p)
				{
					if (GetNamedIdFromJSON (json_p, ST_LOCATION_ID_S, location_id_p))
						{
							bson_oid_t *id_p = GetNewUnitialisedBSONOid ();

							if (id_p)
								{
									if (GetMongoIdFromJSON (json_p, id_p))
										{
											Location *location_p = NULL;
											bool success_flag = true;

											if (! (location_p = GetLocationById (location_id_p, format, data_p)))
												{
													char *id_s = GetBSONOidAsString (location_id_p);

													if (id_s)
														{
															PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, json_p, "GetLocationById failed for %s", id_s);
															FreeBSONOidString (id_s);
														}
													else
														{
															PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, json_p, "GetLocationById failed");
														}

													success_flag = false;
												}


											if (success_flag)
												{
													const char *data_url_s = GetJSONString (json_p, ST_DATA_LINK_S);
													const char *slope_s = GetJSONString (json_p, ST_SLOPE_S);
													const char *aspect_s = GetJSONString (json_p, ST_ASPECT_S);
													const char *description_s = GetJSONString (json_p, ST_DESCRIPTION_S);
													const char *design_s = GetJSONString (json_p, ST_DESIGN_S);
													const char *growing_conditions_s = GetJSONString (json_p, ST_GROWING_CONDITIONS_S);
													const char *phenotype_gathering_notes_s = GetJSONString (json_p, ST_PHENOTYPE_GATHERING_NOTES_S);
													const char *plan_changes_s = GetJSONString (json_p, ST_PLAN_CHANGES_S);
													const char *data_not_included_s = GetJSONString (json_p, ST_DATA_NOT_INCLUDED_S);
													const char *physical_samples_collected_s = GetJSONString (json_p, ST_PHYSICAL_SAMPLES_COLLECTED_S);
													const char *photo_s = GetJSONString (json_p, ST_PHOTO_URL_S);
													const char *image_collection_notes_s = GetJSONString (json_p, ST_IMAGE_COLLECTION_NOTE_S);
													Crop *current_crop_p = GetStoredCropValue (json_p, ST_CURRENT_CROP_S, data_p);
													Crop *previous_crop_p = GetStoredCropValue (json_p, ST_PREVIOUS_CROP_S, data_p);
													const KeyValuePair *aspect_p = NULL;
													double64 *plot_width_p = NULL;
													double64 *plot_length_p = NULL;
													uint32 *num_plot_rows_p = NULL;
													uint32 *num_plot_columns_p = NULL;
													uint32 *num_replicates_p = NULL;
													double64 *plot_horizontal_gap_p = NULL;
													double64 *plot_vertical_gap_p = NULL;
													uint32 *plot_rows_per_block_p = NULL;
													uint32 *plots_columns_per_block_p = NULL;
													double64 *plot_block_horizontal_gap_p = NULL;
													double64 *plot_block_vertical_gap_p = NULL;
													uint32 *sowing_year_p = NULL;
													uint32 *harvest_year_p = NULL;
													uint32 sow = 0;
													uint32 har = 0;

													const char *weather_s = GetJSONString (json_p, ST_WEATHER_S);
													const char *gps_notes_s = GetJSONString (json_p, ST_SHAPE_NOTES_S);


													Person *curator_p = GetPersonFromCompoundJSON (json_p, ST_CURATOR_S, format, data_p);
													Person *contact_p = GetPersonFromCompoundJSON (json_p, ST_CONTACT_S, format, data_p);

													const json_t *shape_p = json_object_get (json_p, ST_SHAPE_S);

													Metadata *metadata_p = GetMetadataFromDefaultChildJSON (json_p, & (data_p -> dftsd_base_data));

													/* use NULL rather than json's the_null */
													if (shape_p == json_null ())
														{
															shape_p = NULL;
														}

													if (aspect_s)
														{
															aspect_p = GetAspect (aspect_s);

															if (aspect_p)
																{
																	aspect_s = aspect_p -> kvp_value_s;
																}
															else
																{
																	aspect_s = NULL;
																}
														}


													GetValidUnsignedIntFromJSON (json_p, ST_NUMBER_OF_PLOT_ROWS_S, &num_plot_rows_p);
													GetValidUnsignedIntFromJSON (json_p, ST_NUMBER_OF_PLOT_COLUMN_S, &num_plot_columns_p);
													GetValidUnsignedIntFromJSON (json_p, ST_NUMBER_OF_REPLICATES_S, &num_replicates_p);

													GetValidRealFromJSON (json_p, ST_PLOT_WIDTH_S, &plot_width_p);
													GetValidRealFromJSON (json_p, ST_PLOT_LENGTH_S, &plot_length_p);

													GetValidRealFromJSON (json_p, ST_PLOT_H_GAP_S, &plot_horizontal_gap_p);
													GetValidRealFromJSON (json_p, ST_PLOT_V_GAP_S, &plot_vertical_gap_p);

													GetValidUnsignedIntFromJSON (json_p, ST_PLOT_ROWS_PER_BLOCK_S, &plot_rows_per_block_p);
													GetValidUnsignedIntFromJSON (json_p, ST_PLOT_COLS_PER_BLOCK_S, &plots_columns_per_block_p);

													GetValidRealFromJSON (json_p, ST_PLOT_BLOCK_H_GAP_S, &plot_block_horizontal_gap_p);
													GetValidRealFromJSON (json_p, ST_PLOT_BLOCK_V_GAP_S, &plot_block_vertical_gap_p);


													if (SetDateFromStudyJSON (json_p, ST_SOWING_YEAR_S, &sow, "sowing_date"))
														{
															sowing_year_p = &sow;
														}

													if (SetDateFromStudyJSON (json_p, ST_HARVEST_YEAR_S, &har, "harvest_date"))
														{
															harvest_year_p = &har;
														}


													study_p = AllocateStudy (id_p, metadata_p, name_s, data_url_s, aspect_s, slope_s, location_p, parent_trial_p, MF_SHALLOW_COPY, current_crop_p, previous_crop_p,
																									 description_s, design_s, growing_conditions_s, phenotype_gathering_notes_s,
																									 num_plot_rows_p, num_plot_columns_p, num_replicates_p, plot_width_p, plot_length_p,
																									 weather_s, shape_p, plot_horizontal_gap_p, plot_vertical_gap_p, plot_rows_per_block_p, plots_columns_per_block_p, plot_block_horizontal_gap_p,
																									 plot_block_vertical_gap_p,
																									 curator_p, contact_p,
																									 sowing_year_p, harvest_year_p,
																									 plan_changes_s, physical_samples_collected_s, data_not_included_s,
																									 photo_s, image_collection_notes_s,
																									 gps_notes_s,
																									 data_p);


													if (study_p)
														{
															const json_t *accessions_p = json_object_get (json_p, ST_ACCESSIONS_S);
															const json_t *contributors_json_p = json_object_get (json_p, ST_CONTRIBUTORS_S);

															if (!AddStatisticsFromJSON (study_p, json_p, data_p))
																{
																	PrintJSONToErrors (STM_LEVEL_WARNING, __FILE__, __LINE__, json_p, "AddStatistcsFromJSON () failed");
																}


															if (!AddTreatmentsFromJSON (study_p, json_p, data_p))
																{
																	PrintJSONToErrors (STM_LEVEL_WARNING, __FILE__, __LINE__, json_p, "AddTreatmentsFromJSON () failed");
																}


															if (contributors_json_p)
																{
																	if (!AddPeopleFromJSON (contributors_json_p, AddPersonFromJSON, study_p, data_p))
																		{
																			PrintJSONToErrors (STM_LEVEL_WARNING, __FILE__, __LINE__, json_p, "AddPeopleFromJSON () failed");
																		}
																}

															if (accessions_p)
																{
																	if (study_p -> st_accessions_p)
																		{
																			json_decref (study_p -> st_accessions_p);
																		}

																	study_p -> st_accessions_p = accessions_p;
																}
														}


													if (num_plot_rows_p)
														{
															FreeMemory (num_plot_rows_p);
														}

													if (num_plot_columns_p)
														{
															FreeMemory (num_plot_columns_p);
														}

													if (num_replicates_p)
														{
															FreeMemory (num_replicates_p);
														}

													if (plot_width_p)
														{
															FreeMemory (plot_width_p);
														}

													if (plot_length_p)
														{
															FreeMemory (plot_length_p);
														}

													if (plot_horizontal_gap_p)
														{
															FreeMemory (plot_horizontal_gap_p);
														}

													if (plot_vertical_gap_p)
														{
															FreeMemory (plot_vertical_gap_p);
														}

													if (plot_rows_per_block_p)
														{
															FreeMemory (plot_rows_per_block_p);
														}

													if (plots_columns_per_block_p)
														{
															FreeMemory (plots_columns_per_block_p);
														}

													if (plot_block_horizontal_gap_p)
														{
															FreeMemory (plot_block_horizontal_gap_p);
														}

													if (plot_block_vertical_gap_p)
														{
															FreeMemory (plot_block_vertical_gap_p);
														}

													/*
													 * If the Study wasn't allocated, free any allocated
													 * resources.
													 */
													if (!study_p)
														{
															PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, json_p, "AllocateStudy failed");

															if (current_crop_p)
																{
																	FreeCrop (current_crop_p);
																}

															if (previous_crop_p)
																{
																	FreeCrop (previous_crop_p);
																}

															if (contact_p)
																{
																	FreePerson (contact_p);
																}

															if (curator_p)
																{
																	FreePerson (curator_p);
																}

														}		/* if (!study_p) */

												}		/* if (success_flag) */

										}		/* if (GetMongoIdFromJSON (json_p, id_p)) */
									else
										{
											PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, json_p, "GetMongoIdFromJSON failed to get %s", ST_LOCATION_S);
										}

									if (!study_p)
										{
											FreeBSONOid (id_p);
										}

								}
							else
								{
									PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "GetNewUnitialisedBSONOid failed for the study id for %s", name_s);
								}

						}		/* if (GetNamedIdFromJSON (json_p, ST_LOCATION_S, address_id_p)) */
					else
						{
							PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, json_p, "GetNamedIdFromJSON failed to get %s", ST_LOCATION_S);
						}


					FreeBSONOid (location_id_p);

				}		/* if (location_id_p) */
			else
				{
					PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "GetNewUnitialisedBSONOid failed for location for %s", name_s);
				}


		}		/* if (name_s) */
	else
		{
			PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, json_p, "Failed to get %s", ST_NAME_S);
		}

	return study_p;

}


Study *GetStudyFromJSON (const json_t *json_p, const ViewFormat format, const FieldTrialServiceData *data_p)
{
	Study *study_p = NULL;
	bson_oid_t *parent_field_trial_id_p = GetNewUnitialisedBSONOid ();

	if (parent_field_trial_id_p)
		{
			if (GetNamedIdFromJSON (json_p, ST_PARENT_FIELD_TRIAL_S, parent_field_trial_id_p))
				{
					FieldTrial *trial_p = GetFieldTrialById (parent_field_trial_id_p, format, data_p);


					if (trial_p)
						{
							study_p = GetStudyWithParentTrialFromJSON (json_p, trial_p, format, data_p);

							if (!study_p)
								{
									FreeFieldTrial (trial_p);
								}
						}
					else
						{
							char *id_s = GetBSONOidAsString (parent_field_trial_id_p);

							if (id_s)
								{
									PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, json_p, "GetFieldTrialById failed for %s", id_s);
									FreeBSONOidString (id_s);
								}
							else
								{
									PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, json_p, "GetFieldTrialById failed");
								}

						}

				}		/* if (GetNamedIdFromJSON (json_p, ST_PARENT_FIELD_TRIAL_S, parent_field_trial_id_p)) */
			else
				{
					PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, json_p, "GetNamedIdFromJSON failed to get %s", ST_PARENT_FIELD_TRIAL_S);
				}

			FreeBSONOid (parent_field_trial_id_p);

		}		/* if (parent_field_trial_id_p) */
	else
		{
			PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, json_p, "GetNewUnitialisedBSONOid failed for parent field trial");
		}

	return study_p;
}


Study *GetStudyByIdString (const char *study_id_s, const ViewFormat format, const FieldTrialServiceData *data_p)
{
	Study *study_p = GetDFWObjectByIdString (study_id_s, DFTD_STUDY, GetStudyCallback, format, data_p);

	return study_p;
}


Study *GetStudyById (bson_oid_t *study_id_p, const ViewFormat format, const FieldTrialServiceData *data_p)
{
	Study *study_p = GetDFWObjectById (study_id_p, DFTD_STUDY, GetStudyCallback, format, data_p);

	return study_p;
}


bool HasStudyGotPlotLayoutDetails (const Study *study_p)
{
	return ((study_p -> st_num_rows_p) && (*study_p -> st_num_rows_p > 0) && (study_p -> st_num_columns_p) && (*study_p -> st_num_columns_p > 0));
}



bool IsMeasuredVariableOnStudy (const Study * const study_p, const char *mv_s)
{
	if (study_p && (study_p -> st_phenotypes_p))
		{
			PhenotypeStatisticsNode *node_p = (PhenotypeStatisticsNode *) (study_p -> st_phenotypes_p -> ll_head_p);

			while (node_p)
				{
					if (strcmp (node_p -> psn_measured_variable_name_s, mv_s) == 0)
						{
							return true;
						}
					else
						{
							node_p = (PhenotypeStatisticsNode *) (node_p -> psn_node.ln_next_p);
						}
				}
		}

	return false;
}



static bool AddValidAspectToJSON (const Study *study_p, json_t *study_json_p)
{
	bool success_flag = true;

	/*
	 * Is the aspect a valid set value?
	 */
	if (! ((IsStringEmpty (study_p -> st_aspect_s)) || (strcmp (study_p -> st_aspect_s, ST_UNKNOWN_DIRECTION_S) == 0)))
		{
			if (!SetJSONString (study_json_p, ST_ASPECT_S, study_p -> st_aspect_s))
				{
					PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, study_json_p, "Failed to add \"%s\": \"%s\"", ST_ASPECT_S, study_p -> st_aspect_s);
					success_flag = false;
				}
		}

	return success_flag;
}



static bool AddValidCropToStorageJSON (Crop *crop_p, json_t *study_json_p, const char * const key_s, const ViewFormat format, const FieldTrialServiceData *data_p)
{
	bool success_flag = false;

	if (crop_p)
		{
			json_t *crop_json_p = GetCropAsJSON (crop_p, format, data_p);

			if (crop_json_p)
				{
					if (json_object_set_new (study_json_p, key_s, crop_json_p) == 0)
						{
							success_flag = true;
						}
					else
						{
							json_decref (crop_json_p);
							PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, crop_json_p, "Failed to add crop to study");
						}
				}
		}
	else
		{
			if (json_object_set_new (study_json_p, key_s, json_null ()) == 0)
				{
					success_flag = true;
				}
		}

	return success_flag;
}




static bool AddValidCropToJSON (Crop *crop_p, json_t *study_json_p, const char * const key_s, const ViewFormat format, const FieldTrialServiceData *data_p)
{
	bool success_flag = false;

	if (crop_p)
		{
			if (format == VF_STORAGE)
				{
					if (AddNamedCompoundIdToJSON (study_json_p, crop_p -> cr_id_p, key_s))
						{
							success_flag = true;
						}
					else
						{
							PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to add crop \"%s\" to study", crop_p -> cr_name_s);
						}
				}
			else
				{
					json_t *crop_json_p = crop_json_p = GetCropAsJSON (crop_p, format, data_p);

					if (crop_json_p)
						{
							if (json_object_set_new (study_json_p, key_s, crop_json_p) == 0)
								{
									success_flag = true;
								}
							else
								{
									json_decref (crop_json_p);
									PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, crop_json_p, "Failed to add crop to study");
								}
						}
				}
		}
	else
		{
			if (json_object_set_new (study_json_p, key_s, json_null ()) == 0)
				{
					success_flag = true;
				}
		}

	return success_flag;
}



static void *GetStudyCallback (const json_t *json_p, const ViewFormat format, const FieldTrialServiceData *data_p)
{
	return GetStudyFromJSON (json_p, format, data_p);
}


static bool AddPlotsToJSON (Study *study_p, json_t *study_json_p, const ViewFormat format, JSONProcessor *processor_p, const FieldTrialServiceData *data_p)
{
	bool success_flag = false;
	json_t *plots_json_p = json_array ();

	if (plots_json_p)
		{
			if (json_object_set_new (study_json_p, ST_PLOTS_S, plots_json_p) == 0)
				{
					PlotNode *node_p = (PlotNode *) (study_p -> st_plots_p -> ll_head_p);

					success_flag = true;

					while (node_p && success_flag)
						{
							json_t *plot_json_p = ProcessPlotJSON (processor_p, node_p -> pn_plot_p, format, data_p);

							if (plot_json_p)
								{
									if (json_array_append_new (plots_json_p, plot_json_p) == 0)
										{
											node_p = (PlotNode *) (node_p -> pn_node.ln_next_p);
										}
									else
										{
											success_flag = false;
											PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, plot_json_p, "Failed to add plot json to results");
										}
								}
							else
								{
									char id_s [MONGO_OID_STRING_BUFFER_SIZE];

									success_flag = false;
									bson_oid_to_string (node_p -> pn_plot_p -> pl_id_p, id_s);

									PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, plot_json_p, "Failed to create plot json for \"%s\"", id_s);
								}

						}		/* while (node_p && &success_flag) */

				}
			else
				{
					PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to add plots array");
					json_decref (plots_json_p);
				}
		}
	else
		{
			PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to create plots array");
		}

	return success_flag;
}




int64 GetNumberOfPlotsInStudy (const Study *study_p, const FieldTrialServiceData *data_p)
{
	int64 res = -1;

	if (SetMongoToolCollection (data_p -> dftsd_mongo_p, data_p -> dftsd_collection_ss [DFTD_PLOT]))
		{
			bson_t *query_p = BCON_NEW (PL_PARENT_STUDY_S, BCON_OID (study_p -> st_id_p));

			/*
			 * Make the query to get the matching plots
			 */
			if (query_p)
				{
					res = GetNumberOfMongoResults (data_p -> dftsd_mongo_p, query_p, NULL);
					//					json_t *results_p = GetAllMongoResultsAsJSON (data_p -> dftsd_mongo_p, query_p, NULL);
					//
					//					if (results_p)
					//						{
					//							if (json_is_array (results_p))
					//								{
					//									res = json_array_size (results_p);
					//								}		/* if (json_is_array (results_p)) */
					//
					//							json_decref (results_p);
					//						}		/* if (results_p) */

					bson_destroy (query_p);
				}		/* if (query_p) */

		}

	return res;
}



/*
 * For Client formats
 */
static bool AddParentFieldTrialToJSON (Study *study_p, json_t *study_json_p, const FieldTrialServiceData *data_p)
{
	if (study_p -> st_parent_p)
		{
			json_t *field_trial_json_p = GetFieldTrialAsJSON (study_p -> st_parent_p, VF_CLIENT_MINIMAL, data_p);

			if (field_trial_json_p)
				{
					if (json_object_set_new (study_json_p, ST_PARENT_FIELD_TRIAL_S, field_trial_json_p) == 0)
						{
							return true;
						}

					json_decref (field_trial_json_p);
				}
		}

	return false;
}


/*
 * For Client formats
 */
static bool AddGrandParentProgramToJSON (Study *study_p, json_t *study_json_p, const ViewFormat format, const FieldTrialServiceData *data_p)
{
	FieldTrial *trial_p = study_p -> st_parent_p;

	if (trial_p)
		{
			Programme *program_p = trial_p -> ft_parent_p;

			if (program_p)
				{
					json_t *program_json_p = GetProgrammeAsJSON (program_p, format, data_p);

					if (program_json_p)
						{
							if (json_object_set_new (study_json_p, FT_PARENT_PROGRAM_S, program_json_p) == 0)
								{
									return true;
								}
							else
								{
									PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, program_json_p, "Failed to add program to study \"%s\"", study_p -> st_name_s);
									json_decref (program_json_p);
								}
						}
					else
						{
							PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to get program \"%s\" as json", program_p  -> pr_name_s);
						}
				}
			else
				{
					PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "no program for trial \"%s\"", trial_p -> ft_name_s);
				}
		}
	else
		{
			PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "no parent program for study \"%s\"", study_p -> st_name_s);
		}

	return false;
}



static bool AddDefaultPlotValuesToJSON (const Study *study_p, json_t *study_json_p, const FieldTrialServiceData *data_p)
{
	if (SetNonTrivialDouble (study_json_p, ST_PLOT_WIDTH_S, study_p -> st_default_plot_width_p, true))
		{
			if (SetNonTrivialDouble (study_json_p, ST_PLOT_LENGTH_S, study_p -> st_default_plot_length_p, true))
				{
					if (SetNonTrivialUnsignedInt (study_json_p, ST_NUMBER_OF_PLOT_ROWS_S, study_p -> st_num_rows_p, true))
						{
							if (SetNonTrivialUnsignedInt (study_json_p, ST_NUMBER_OF_PLOT_COLUMN_S, study_p -> st_num_columns_p, true))
								{
									if (SetNonTrivialUnsignedInt (study_json_p, ST_NUMBER_OF_REPLICATES_S, study_p -> st_num_replicates_p, true))
										{
											return true;
										}
								}
						}
				}
		}

	return false;
}


static bool AddTreatmentsToJSON (const Study *study_p, json_t *study_json_p, const ViewFormat format)
{
	bool success_flag = false;

	if (study_p -> st_treatments_p)
		{
			json_t *treatments_json_p = json_array ();

			if (treatments_json_p)
				{
					bool b = true;
					TreatmentFactorNode *node_p = (TreatmentFactorNode *) (study_p -> st_treatments_p -> ll_head_p);

					while (node_p && b)
						{
							json_t *treatment_json_p = GetTreatmentFactorAsJSON (node_p -> tfn_p, format);

							if (treatment_json_p)
								{
									if (json_array_append_new (treatments_json_p, treatment_json_p) == 0)
										{
											node_p = (TreatmentFactorNode *) (node_p -> tfn_node.ln_next_p);
										}
									else
										{
											json_decref (treatment_json_p);
											b = false;
										}
								}
							else
								{
									b = false;
								}

						}

					if (json_array_size (treatments_json_p) == study_p -> st_treatments_p -> ll_size)
						{
							if (json_object_set_new (study_json_p, ST_TREATMENTS_S, treatments_json_p) == 0)
								{
									return true;
								}
						}

					json_decref (treatments_json_p);
				}		/* if (treatments_json_p) */

		}		/* if (study_p -> st_treatments_p) */
	else
		{
			return true;
		}

	return false;
}


static bool AddCommonStudyJSONValues (Study *study_p, json_t *study_json_p, const ViewFormat format, const FieldTrialServiceData *data_p)
{
	bool success_flag = false;

	if (SetJSONString (study_json_p, ST_NAME_S, study_p -> st_name_s))
		{
			if (SetNonTrivialString (study_json_p, ST_DESCRIPTION_S, study_p -> st_description_s, true))
				{
					if (SetNonTrivialString (study_json_p, ST_GROWING_CONDITIONS_S, study_p -> st_growing_conditions_s, true))
						{
							if (SetNonTrivialString (study_json_p, ST_DESIGN_S, study_p -> st_design_s, true))
								{
									if (SetNonTrivialString (study_json_p, ST_GROWING_CONDITIONS_S, study_p -> st_growing_conditions_s, true))
										{
											if (SetNonTrivialString (study_json_p, ST_PHENOTYPE_GATHERING_NOTES_S, study_p -> st_phenotype_gathering_notes_s, true))
												{
													if (SetNonTrivialString (study_json_p, ST_DATA_LINK_S, study_p -> st_data_url_s, true))
														{
															if (SetNonTrivialString (study_json_p, ST_SLOPE_S, study_p -> st_slope_s, true))
																{
																	if (SetNonTrivialString (study_json_p, ST_WEATHER_S, study_p -> st_weather_link_s, true))
																		{
																			if (SetNonTrivialString (study_json_p, ST_PLAN_CHANGES_S, study_p -> st_plan_changes_s, true))
																				{
																					if (SetNonTrivialString (study_json_p, ST_DATA_NOT_INCLUDED_S, study_p -> st_data_not_included_s, true))
																						{
																							if (SetNonTrivialString (study_json_p, ST_PHYSICAL_SAMPLES_COLLECTED_S, study_p -> st_physical_samples_collected_s, true))
																								{
																									if (SetNonTrivialString (study_json_p, ST_PHOTO_URL_S, study_p -> st_photo_url_s, true))
																										{
																											if (SetNonTrivialString (study_json_p, ST_IMAGE_COLLECTION_NOTE_S, study_p -> st_image_collection_notes_s, true))
																												{
																													if (SetNonTrivialString (study_json_p, ST_SHAPE_NOTES_S, study_p -> st_shape_notes_s, true))
																														{
																															if (AddValidAspectToJSON (study_p, study_json_p))
																																{
																																	if (AddCommonPlotValuesToJSON (study_p, study_json_p, format, data_p))
																																		{
																																			if (AddTreatmentsToJSON (study_p, study_json_p, format))
																																				{
																																					if (SetNonTrivialUnsignedInt (study_json_p, ST_SOWING_YEAR_S, study_p -> st_predicted_sowing_year_p, true))
																																						{
																																							if (SetNonTrivialUnsignedInt (study_json_p, ST_HARVEST_YEAR_S, study_p -> st_predicted_harvest_year_p, true))
																																								{
																																									if ((study_p -> st_curator_p == NULL) || (AddPersonToCompoundJSON (study_p -> st_curator_p, study_json_p, ST_CURATOR_S, format, data_p)))
																																										{
																																											if ((study_p -> st_contact_p == NULL) || (AddPersonToCompoundJSON (study_p -> st_contact_p, study_json_p, ST_CONTACT_S, format, data_p)))
																																												{
																																													if (AddPhenotypesToJSON (study_p, study_json_p, format, data_p))
																																														{
																																															if (AddAccessionsToJSON (study_p, study_json_p, format, data_p))
																																																{
																																																	if (AddPeopleToJSON (study_p -> st_contributors_p, ST_CONTRIBUTORS_S, study_json_p, format, data_p))
																																																		{
																																																			success_flag = true;
																																																		}
																																																	else
																																																		{
																																																			PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, study_json_p, "Failed to add contributors to study \"%s\"", study_p -> st_name_s);
																																																		}
																																																}
																																														}
																																													else
																																														{
																																															PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, study_json_p, "Failed to add statistics to study \"%s\"", study_p -> st_parent_p -> ft_name_s);
																																														}

																																												}

																																										}

																																								}

																																						}

																																				}		/* if (AddTreatmentsToJSON (study_p, study_json_p, format)) */

																																		}		/* if (AddCommonPlotValuesToJSON (study_p, study_json_p, format, data_p)) */


																																}		/* if (AddValidAspectToJSON (study_p, study_json_p)) */
																														}		/* if (SetNonTrivialString (study_json_p, ST_SHAPE_NOTES_S, study_p -> st_shape_notes_s, true)) */

																												}
																										}




																								}		/* if (SetNonTrivialString (study_json_p, ST_PHYSICAL_SAMPLES_COLLECTED_S, study_p -> st_physical_samples_collected_s, true)) */

																						}		/* if (SetNonTrivialString (study_json_p, ST_DATA_NOT_INCLUDED_S, study_p -> st_data_not_included_s, true)) */

																				}		/* if (SetNonTrivialString (study_json_p, ST_PLAN_CHANGES_S, study_p -> st_plan_changes_s, true)) */


																		}		/* if (SetNonTrivialString (study_json_p, ST_WEATHER_S, study_p -> st_weather_link_s)) */

																}		/* if ((IsStringEmpty (study_p -> st_slope_s)) || (SetJSONString (study_json_p, ST_SLOPE_S, study_p -> st_slope_s))) */


														}		/* if ((IsStringEmpty (study_p -> st_data_url_s)) || (SetJSONString (study_json_p, ST_DATA_LINK_S, study_p -> st_data_url_s) == 0)) */

												}
										}
								}

						}

				}		/* if ((study_p -> st_min_ph == ST_UNSET_PH) || (SetJSONInteger (study_json_p, ST_MIN_PH_S, study_p -> st_min_ph))) */

		}		/* if (SetJSONString (study_json_p, ST_NAME_S, study_p -> st_name_s) == 0) */

	return success_flag;
}


static bool AddCommonPlotValuesToJSON (Study *study_p, json_t *study_json_p, const ViewFormat format, const FieldTrialServiceData *data_p)
{
	bool success_flag = false;

	if (AddDefaultPlotValuesToJSON (study_p, study_json_p, data_p))
		{
			if (SetNonTrivialDouble (study_json_p, ST_PLOT_H_GAP_S, study_p -> st_plot_horizontal_gap_p, true))
				{
					if (SetNonTrivialDouble (study_json_p, ST_PLOT_V_GAP_S, study_p -> st_plot_vertical_gap_p, true))
						{
							if (SetNonTrivialUnsignedInt (study_json_p, ST_PLOT_ROWS_PER_BLOCK_S, study_p -> st_plots_rows_per_block_p, true))
								{
									if (SetNonTrivialUnsignedInt (study_json_p, ST_PLOT_COLS_PER_BLOCK_S, study_p -> st_plots_columns_per_block_p, true))
										{
											if (SetNonTrivialDouble (study_json_p, ST_PLOT_BLOCK_H_GAP_S, study_p -> st_plot_block_horizontal_gap_p, true))
												{
													if (SetNonTrivialDouble (study_json_p, ST_PLOT_BLOCK_V_GAP_S, study_p -> st_plot_block_vertical_gap_p, true))
														{
															success_flag = true;
														}

												}

										}

								}

						}

				}

		}		/* if (AddDefaultPlotValuesToJSON (study_p, study_json_p, data_p)) */
	else
		{
			PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, study_json_p, "AddDefaultPlotValuesToJSON failed for study \"%s\"", study_p -> st_parent_p -> ft_name_s);
		}

	return success_flag;
}


static bool AddTreatmentsFromJSON (Study *study_p, const json_t *study_json_p, const FieldTrialServiceData *data_p)
{
	bool success_flag = true;
	const json_t *treatment_factors_json_p = json_object_get (study_json_p, ST_TREATMENTS_S);

	if (treatment_factors_json_p)
		{
			size_t i;
			const size_t size = json_array_size (treatment_factors_json_p);

			for (i = 0; i < size; ++ i)
				{
					const json_t *treatment_factor_json_p = json_array_get (treatment_factors_json_p, i);
					TreatmentFactor *tf_p = GetTreatmentFactorFromJSON (treatment_factor_json_p, study_p, data_p);

					if (tf_p)
						{
							if (!AddTreatmentFactorToStudy (tf_p, study_p))
								{
									i = size;		/* force exit from loop */
								}
						}
					else
						{
							i = size;		/* force exit from loop */
						}

				}		/* for (i = 0; i < size; ++ i) */

			if (study_p -> st_treatments_p -> ll_size != size)
				{
					success_flag = false;
				}

		}		/* if (treatments_json_p) */

	return success_flag;
}


static bool AddFrictionlessDataLink (const Study * const study_p, json_t *study_json_p, const FieldTrialServiceData *data_p)
{
	bool success_flag = false;
	char *full_study_filename_s = GetFrictionlessDataFilename (study_p -> st_name_s, data_p);

	if (full_study_filename_s)
		{
			if (DoesFileExist (full_study_filename_s))
				{
					char *fd_url_s = GetFrictionlessDataURL (study_p -> st_name_s, data_p);

					if (fd_url_s)
						{
							if (SetJSONString (study_json_p, ST_FRICTIONLESS_DATA_LINK_S, fd_url_s))
								{
									success_flag = true;
								}

							FreeCopiedString (fd_url_s);
						}

				}
			else
				{
					PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Frictionless Data file does not exist for study \"%s\"", study_p -> st_name_s);
				}

			FreeCopiedString (full_study_filename_s);
		}


	return success_flag;
}



static bool AddHandbookLinks (const Study * const study_p, json_t *study_json_p, const FieldTrialServiceData *data_p)
{
	bool success_flag = false;
	char *full_study_filename_s = GetStudyHandbookFilename (study_p, data_p);

	if (full_study_filename_s)
		{
			if (DoesFileExist (full_study_filename_s))
				{
					char *handbook_url_s = GetStudyHandbookURL (study_p -> st_name_s, data_p);

					if (handbook_url_s)
						{
							json_t *handbook_p = json_object ();

							if (handbook_p)
								{
									const char * const type_s = CONTEXT_PREFIX_SCHEMA_ORG_S "DigitalDocument";

									if (SetJSONString (handbook_p, "@type", type_s))
										{
											if (SetJSONString (handbook_p, "name", study_p -> st_name_s))
												{
													const char * const url_key_s = CONTEXT_PREFIX_SCHEMA_ORG_S "url";

													if (SetJSONString (handbook_p, url_key_s, handbook_url_s))
														{
															if (json_object_set_new (study_json_p, ST_HANDBOOK_DATA_LINK_S, handbook_p) == 0)
																{
																	success_flag = true;
																}
															else
																{
																	PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, handbook_p, "Failed to add handbook object to study json for \"%s\"", study_p -> st_name_s);
																}

														}		/* if (SetJSONString (handbook_p, url_key_s, handbook_url_s)) */
													else
														{
															PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, handbook_p, "SetJSONString () failed study \"%s\" adding \"%s\": \"%s\"", url_key_s, handbook_url_s);
														}
												}
											else
												{
													PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, handbook_p, "SetJSONString () failed study \"%s\" adding \"%s\": \"%s\"", "name", study_p -> st_name_s);
												}

										}
									else
										{
											PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, handbook_p, "SetJSONString () failed study \"%s\" adding \"%s\": \"%s\"", "@type", type_s);
										}

									if (!success_flag)
										{
											json_decref (handbook_p);
										}

								}		/* if (handbook_p) */
							else
								{
									PrintErrors (STM_LEVEL_INFO, __FILE__, __LINE__, "Failed to allocate handbook metadata json object for study \"%s\"", study_p -> st_name_s);
								}

							FreeCopiedString (handbook_url_s);
						}		/* if (handbook_url_s) */
					else
						{
							PrintErrors (STM_LEVEL_INFO, __FILE__, __LINE__, "GetStudyHandbookURL () failed for study \"%s\"", study_p -> st_name_s);
						}

				}		/* if (DoesFileExist (full_study_filename_s)) */
			else
				{
					PrintErrors (STM_LEVEL_INFO, __FILE__, __LINE__, "Handbook file does not exist for study \"%s\"", study_p -> st_name_s);
				}

			FreeCopiedString (full_study_filename_s);
		}		/* if (full_study_filename_s) */
	else
		{
			PrintErrors (STM_LEVEL_INFO, __FILE__, __LINE__, "GetStudyHandbookFilename () failed for study \"%s\"", study_p -> st_name_s);
		}


	return success_flag;
}



static bool AddAccessionsToJSON (Study *study_p, json_t *study_json_p, const ViewFormat format, const FieldTrialServiceData *data_p)
{
	bool success_flag = false;


	if (study_p -> st_accessions_p)
		{

		}
	else if ((study_p -> st_plots_p) && (study_p -> st_plots_p -> ll_size > 0))
		{
			/*
			 * Are there any phenotypes?
			 */

			json_t *accessions_p = json_object ();

			if (accessions_p)
				{
					PlotNode *plot_node_p = (PlotNode *) (study_p -> st_plots_p -> ll_head_p);

					success_flag = true;

					/*
					 * get all of the unique accession names
					 */
					while (plot_node_p && success_flag)
						{
							Plot *plot_p = plot_node_p -> pn_plot_p;

							if ((plot_p -> pl_rows_p) && (plot_p -> pl_rows_p -> ll_size > 0))
								{
									RowNode *row_node_p = (RowNode *) (plot_p -> pl_rows_p -> ll_head_p);

									if (row_node_p -> rn_row_p -> ro_type == RT_STANDARD)
										{
											StandardRow *row_p = (StandardRow *) (row_node_p -> rn_row_p);

											if (row_p -> sr_material_p)
												{
													const char *accession_s = row_p -> sr_material_p -> ma_accession_s;

													if (accession_s)
														{
															json_int_t count = 0;

															GetJSONInteger (accessions_p, ST_ACCESSIONS_S, &count);

															++ count;


															if (!SetJSONInteger (accessions_p, ST_ACCESSIONS_S, count))
																{
																	success_flag = false;
																}
														}

												}
										}

									row_node_p = (RowNode *) (row_node_p -> rn_node.ln_next_p);
								}


							plot_node_p = (PlotNode *) (plot_node_p -> pn_node.ln_next_p);
						}		/* while (plot_node_p && success_flag) */

					if (success_flag)
						{
							if (study_p -> st_accessions_p)
								{
									json_decref (study_p -> st_accessions_p);
								}

							study_p -> st_accessions_p = accessions_p;
						}
					else
						{
							json_decref (accessions_p);
						}
				}		/* if (accessions_p) */
			else
				{
					PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, study_json_p, "Failed to create phenotypes object for \"%s\"", study_p -> st_name_s);
				}

		}		/* if ((study_p -> st_phenotype_statistics_p) && (study_p -> st_phenotype_statistics_p -> ll_size > 0)) */
	else
		{
			/* no phenotypes to do */
			success_flag = true;
		}

	return success_flag;
}


bool AddPhenotypesToJSON (const Study *study_p, json_t *study_json_p, const ViewFormat format, const FieldTrialServiceData *data_p)
{
	bool success_flag = false;

	/*
	 * Are there any phenotypes?
	 */
	if ((study_p -> st_phenotypes_p) && (study_p -> st_phenotypes_p -> ll_size > 0))
		{
			json_t *phenotypes_p = json_object ();

			if (phenotypes_p)
				{
					bool b = true;
					PhenotypeStatisticsNode *node_p = (PhenotypeStatisticsNode *) (study_p -> st_phenotypes_p -> ll_head_p);
					uint32 i = 0;

					/*
					 * Keep looping and adding the phenotypes
					 */
					while (node_p && b)
						{
							if (AddPhenotypeStatisticsNodeAsJSON (node_p, phenotypes_p, format, data_p))
								{
									node_p = (PhenotypeStatisticsNode *) (node_p -> psn_node.ln_next_p);
									++ i;
								}
							else
								{
									b = false;
									PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, study_json_p, "AddPhenotypeStatisticsNodeAsJSON () failed for \"%s\" for study \"%s\"", node_p -> psn_measured_variable_name_s, study_p -> st_name_s);
								}

						}		/* while (node_p && b) */

					/*
					 * Did we process all of the phenotypes?
					 */
					if (i == study_p -> st_phenotypes_p -> ll_size)
						{
							if (json_object_set_new (study_json_p, ST_PHENOTYPES_S, phenotypes_p) == 0)
								{
									success_flag = true;
								}
							else
								{
									PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, study_json_p, "Failed to add statistics for \"%s\" for study \"%s\"", ST_PHENOTYPE_STATISTICS_S, study_p -> st_name_s);
								}
						}
					else
						{
							PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, phenotypes_p, "phenotypes has " UINT32_FMT " entries instead of " UINT32_FMT "  for study \"%s\"", i, study_p -> st_phenotypes_p -> ll_size, study_p -> st_name_s);
						}

					if (!success_flag)
						{
							json_decref (phenotypes_p);
						}

				}		/* if (phenotypes_p) */
			else
				{
					PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, study_json_p, "Failed to create phenotypes object for \"%s\"", study_p -> st_name_s);
				}

		}		/* if ((study_p -> st_phenotype_statistics_p) && (study_p -> st_phenotype_statistics_p -> ll_size > 0)) */
	else
		{
			/* no phenotypes to do */
			success_flag = true;
		}

	return success_flag;
}



bool AddPhenotypeStatisticsToStudy (Study * const study_p, const char *mv_s, const Statistics *stats_p)
{
	bool success_flag = false;
	PhenotypeStatisticsNode *node_p = AllocatePhenotypeStatisticsNode (mv_s, stats_p);

	if (node_p)
		{
			LinkedListAddTail (study_p -> st_phenotypes_p, & (node_p -> psn_node));

			success_flag = true;
		}

	return success_flag;
}



bool CopyAllPhenotypeStatistics (const Study * const src_p, Study *dest_p, const bool remove_existing_entries_flag)
{
	bool success_flag = true;
	PhenotypeStatisticsNode *src_node_p = (PhenotypeStatisticsNode *) (src_p -> st_phenotypes_p ->ll_head_p);


	if (remove_existing_entries_flag)
		{
			ClearLinkedList (dest_p -> st_phenotypes_p);
		}

	while (src_node_p && success_flag)
		{
			if (AddPhenotypeStatisticsToStudy (dest_p, src_node_p -> psn_measured_variable_name_s, src_node_p -> psn_stats_p))
				{
					src_node_p = (PhenotypeStatisticsNode *) (src_node_p -> psn_node.ln_next_p);
				}
			else
				{
					PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "AddPhenotypeStatisticsToStudy () failed copying \"%s\" from \"%s\" to \"%s\"",
											 src_node_p -> psn_measured_variable_name_s, src_p -> st_name_s, dest_p -> st_name_s);

					success_flag = false;
				}
		}

	return success_flag;
}



/*
 * AllocateStudy (bson_oid_t *id_p,  Metadata *metadata_p, const char *name_s, const char *data_url_s, const char *aspect_s, const char *slope_s,
	struct Location *location_p, FieldTrial *parent_field_trial_p,
	MEM_FLAG parent_field_trial_mem, Crop *current_crop_p, Crop *previous_crop_p, const char *description_s,
	const char *design_s, const char *growing_conditions_s, const char *phenotype_gathering_notes_s,
	const uint32 *num_rows_p, const uint32 *num_cols_p, const uint32 *num_replicates_p, const double64 *plot_width_p, const double64 *plot_length_p,
	const char *weather_s, const json_t *shape_p, const double64 *plot_horizontal_gap_p, const double64 *plot_vertical_gap_p,
	const uint32 *plot_rows_per_block_p, const uint32 *plot_columns_per_block_p, const double64 *plot_block_horizontal_gap_p,
	const double64 *plot_block_vertical_gap_p,
	Person *curator_p, Person *contact_p,
	const uint32 *sowing_year_p, const uint32 *harvest_year_p,
	const char *plan_changes_s, const char *physical_samples_collected_s, const char *data_not_included_s,
	const char *photo_url_s, const char *image_collection_notes_s,
	const char *gps_notes_s,
	const FieldTrialServiceData *data_p);
 */
Study *CopyStudy (const Study * const src_p, const char * const new_name_s, const bool copy_treatment_factors_flag, const bool copy_measured_variables_flag, const FieldTrialServiceData *data_p)
{
	Metadata *metadata_p = NULL;
	Location *location_p = NULL;
	Crop *current_crop_p =  NULL;


	location_p = CopyLocation (src_p -> st_location_p);

	if (location_p)
		{
			if ((src_p -> st_current_crop_p == NULL)|| ((current_crop_p = CopyCrop (src_p -> st_current_crop_p)) != NULL))
				{
					Crop *previous_crop_p = NULL;

					if ((src_p -> st_previous_crop_p == NULL)|| ((previous_crop_p = CopyCrop (src_p -> st_previous_crop_p)) != NULL))
						{
							Person *contact_p = NULL;

							if ((src_p -> st_contact_p == NULL)|| ((contact_p = CopyPerson (src_p -> st_contact_p)) != NULL))
								{
									Person *curator_p = NULL;

									if ((src_p -> st_curator_p == NULL)|| ((curator_p = CopyPerson (src_p -> st_curator_p)) != NULL))
										{
											Study *dest_p = AllocateStudy (NULL, metadata_p,  new_name_s, src_p -> st_data_url_s, src_p -> st_aspect_s, src_p -> st_slope_s,
																										 location_p, src_p -> st_parent_p, MF_SHADOW_USE, current_crop_p, previous_crop_p, src_p -> st_description_s,
																										 src_p -> st_design_s, src_p -> st_growing_conditions_s, src_p -> st_phenotype_gathering_notes_s,
																										 src_p -> st_num_rows_p, src_p -> st_num_columns_p, src_p -> st_num_replicates_p, src_p -> st_default_plot_width_p, src_p -> st_default_plot_length_p,
																										 src_p -> st_weather_link_s, src_p -> st_shape_p, src_p -> st_plot_horizontal_gap_p, src_p -> st_plot_vertical_gap_p,
																										 src_p -> st_plots_rows_per_block_p, src_p -> st_plots_columns_per_block_p, src_p -> st_plot_block_horizontal_gap_p, src_p -> st_plot_block_vertical_gap_p,
																										 curator_p, contact_p,
																										 src_p -> st_predicted_sowing_year_p, src_p -> st_predicted_harvest_year_p,
																										 src_p -> st_plan_changes_s, src_p -> st_physical_samples_collected_s, src_p -> st_data_not_included_s,
																										 src_p -> st_photo_url_s, src_p -> st_image_collection_notes_s,
																										 src_p -> st_shape_notes_s,
																										 data_p);

											if (dest_p)
												{
													bool success_flag = true;

													//											LinkedList *st_treatments_p;

		//											LinkedList *st_phenotypes_p;

		//												LinkedList *st_contributors_p;


													/*
													 * Now that the Study has the contact, contributor and crops we
													 * don't need to worry about deleting them to stop memory leaks
													 * as the Study will take care of that
													 */
													contact_p = NULL;
													curator_p = NULL;
													previous_crop_p = NULL;
													current_crop_p = NULL;


													if (copy_treatment_factors_flag)
														{

															if (src_p -> st_treatments_p -> ll_size > 0)
																{
																	/* Copy all of the treatments */
																	const TreatmentFactorNode *src_node_p = (const TreatmentFactorNode *) (src_p -> st_treatments_p -> ll_head_p);

																	while ((src_node_p != NULL) && (success_flag == true))
																		{
																			const TreatmentFactor *src_tf_p = src_node_p -> tfn_p;

																			TreatmentFactor *dest_tf_p = CopyTreatmentFactor (src_node_p -> tfn_p, dest_p);

																			if (dest_tf_p)
																				{
																					if (AddTreatmentFactorToStudy (dest_tf_p, dest_p))
																						{
																							src_node_p = (const TreatmentFactorNode *) (src_node_p -> tfn_node.ln_next_p);
																						}
																					else
																						{
																							success_flag = false;
																							PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "AddTreatmentFactorToStudy () failed");
																						}
																				}

																		}		/* while ((src_node_p != NULL) && (success_flag == true)) */

																}

														}		/* if (copy_treatment_factors_flag) */


													if (success_flag)
														{
															if (copy_measured_variables_flag)
																{

																}		/* if (copy_treatment_factors_flag) */


															if (success_flag)
																{
																	if (src_p -> st_contributors_p -> ll_size > 0)
																		{
																			/* Copy all of the contributors */
																			const PersonNode *src_node_p = (const PersonNode *) (src_p -> st_contributors_p -> ll_head_p);

																			while ((src_node_p != NULL) && (success_flag == true))
																				{
																					Person *contributor_p = CopyPerson (src_node_p -> pn_person_p);

																					if (contributor_p)
																						{
																							if (AddStudyContributor (dest_p, contributor_p, MF_SHALLOW_COPY))
																								{
																									src_node_p = (const PersonNode *) (src_node_p -> pn_node.ln_next_p);
																								}
																							else
																								{
																									success_flag = false;
																									PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "AddStudyContributor () failed for \"%s\" to \"%s\"",
																															 contributor_p -> pe_name_s, dest_p -> st_name_s);

																									FreePerson (contributor_p);
																								}
																						}

																				}		/* while ((src_node_p != NULL) && (success_flag == true)) */

																		}		/* if (src_p -> st_contributors_p -> ll_size > 0) */

																}		/* if (success_flag) */


															if (success_flag)
																{
																	if (src_p -> st_accessions_p)
																		{
																			dest_p -> st_accessions_p = json_deep_copy (src_p -> st_accessions_p);

																			if (! (dest_p -> st_accessions_p))
																				{
																					success_flag = false;
																					PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, src_p -> st_accessions_p, "Failed to copy accessions from  \"%s\" to \"%s\"",
																											 src_p -> st_name_s, dest_p -> st_name_s);
																				}

																		}
																}


															if (success_flag)
																{
																	return dest_p;
																}

														}		/* if (success_flag) */


													FreeStudy (dest_p);
												}		/* if (dest_p) */


											if (curator_p)
												{
													FreePerson (curator_p);
												}
										}		/* if ((src_p -> st_curator_p == NULL)|| ((curator_p = CopyPerson (src_p -> st_curator_p)) != NULL)) */
									else
										{
											PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to copy person \"%s\", \"%s\"", src_p -> st_contact_p -> pe_name_s, src_p -> st_contact_p -> pe_email_s);
										}

									if (contact_p)
										{
											FreePerson (contact_p);
										}

								}		/* if ((src_p -> st_contact_p == NULL)|| ((contact_p = CopyPerson (src_p -> st_contact_p)) != NULL)) */
							else
								{
									PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to copy person \"%s\", \"%s\"", src_p -> st_contact_p -> pe_name_s, src_p -> st_contact_p -> pe_email_s);
								}


							if (previous_crop_p)
								{
									FreeCrop (previous_crop_p);
								}


						}		/* if ((src_p -> st_previous_crop_p == NULL)|| ((previous_crop_p = CopyCrop (src_p -> st_previous_crop_p)) != NULL)) */
					else
						{
							PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to copy crop \"%s\"", src_p -> st_previous_crop_p -> cr_name_s);
						}


					if (current_crop_p)
						{
							FreeCrop (current_crop_p);
						}

				}		/* if ((src_p -> st_current_crop_p == NULL)|| ((current_crop_p = CopyCrop (src_p -> st_current_crop_p)) != NULL)) */
			else
				{
					PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to copy crop \"%s\"", src_p -> st_current_crop_p -> cr_name_s);
				}

		}		/* if (location_p) */
	else
		{
			PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to copy location \"%s\"", src_p -> st_location_p -> lo_address_p -> ad_name_s);
		}

	return NULL;
}



static bool AddStatisticsFromJSON (Study *study_p, const json_t *study_json_p, const FieldTrialServiceData *service_data_p)
{
	bool success_flag = true;
	const json_t *phenotypes_p = json_object_get (study_json_p, ST_PHENOTYPES_S);

	if (phenotypes_p)
		{
			const size_t size = json_object_size (phenotypes_p);
			const char *key_s;
			json_t *phenotype_json_p;
			size_t num_added = 0;

			json_object_foreach (phenotypes_p, key_s, phenotype_json_p)
				{
					if (AddPhenotypeStatisticsNodeFromJSON (study_p, phenotype_json_p, service_data_p))
						{
							++ num_added;
						}
					else
						{
							PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, phenotype_json_p, "AddPhenotypeStatisticsNodeFromJSON () failed for \"%s\"", key_s);
						}


				}		/* json_object_foreach (statistics_p, key_s, value_p) */

			if (num_added != size)
				{
					PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, phenotypes_p, "Failed to get all statistics from json");
					success_flag = false;
				}

		}		/* if (phenotypes_p) */

	return success_flag;
}








static bool SetDateFromStudyJSON (const json_t *json_p, const char *key_s, uint32 *year_p, const char *deprecated_key_s)
{
	bool success_flag = false;

	if (GetJSONUnsignedInteger (json_p, key_s, year_p))
		{
			success_flag = true;
		}
	else
		{
			/*
			 * Have we got it in the old date format?
			 */
			const json_t *deprecated_date_p = json_object_get (json_p, deprecated_key_s);

			if (deprecated_date_p)
				{
					if (json_is_string (deprecated_date_p))
						{
							const char *date_s = json_string_value (deprecated_date_p);

							if (date_s)
								{
									struct tm *time_p = GetTimeFromString (date_s);

									if (time_p)
										{
											*year_p = 1900 + (time_p -> tm_year);

											success_flag = true;
											FreeTime (time_p);
										}
								}

						}
					else if (json_is_integer (deprecated_date_p))
						{
							time_t t = (time_t) json_integer_value (deprecated_date_p);
							struct tm *time_p = gmtime (&t);

							if (time_p)
								{
									*year_p = 1900 + (time_p -> tm_year);
									success_flag = true;
								}
						}
				}		/* if (deprecated_date_p) */
		}

	return success_flag;
}



static bool AddPersonFromJSON (Person *person_p, void *user_data_p, MEM_FLAG *mem_p)
{
	bool success_flag = false;
	Study *study_p = (Study *) user_data_p;
	MEM_FLAG mf = MF_SHALLOW_COPY;

	if (AddStudyContributor (study_p, person_p, mf))
		{
			*mem_p = mf;
			success_flag = true;
		}

	return success_flag;
}
