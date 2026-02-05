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
 * experimental_area.h
 *
 *  Created on: 11 Sep 2018
 *      Author: billy
 */

#ifndef SERVICES_FIELD_TRIALS_INCLUDE_STUDY_H_
#define SERVICES_FIELD_TRIALS_INCLUDE_STUDY_H_


#include <time.h>

#include "dfw_field_trial_service_data.h"
#include "dfw_field_trial_service_library.h"
#include "field_trial.h"
#include "location.h"
#include "jansson.h"
#include "key_value_pair.h"
#include "crop.h"
#include "person.h"

#include "typedefs.h"
#include "address.h"
#include "json_processor.h"
#include "statistics.h"

#include "metadata.h"


#ifndef DOXYGEN_SHOULD_SKIP_THIS

#ifdef ALLOCATE_STUDY_TAGS
	#define STUDY_PREFIX DFW_FIELD_TRIAL_SERVICE_LOCAL
	#define STUDY_VAL(x)	= x
	#define STUDY_CONCAT_VAL(x,y)	= x y
	#define STUDY_KEY_VALUE_PAIR_VAL(x,y) = {x, y}
#else
	#define STUDY_PREFIX extern
	#define STUDY_VAL(x)
	#define STUDY_CONCAT_VAL(x,y)
	#define STUDY_KEY_VALUE_PAIR_VAL(x,y)
#endif

#endif 		/* #ifndef DOXYGEN_SHOULD_SKIP_THIS */

STUDY_PREFIX const char *ST_NAME_S STUDY_CONCAT_VAL (CONTEXT_PREFIX_SCHEMA_ORG_S, "name");

STUDY_PREFIX const char *ST_LOCATION_ID_S STUDY_VAL ("address_id");

STUDY_PREFIX const char *ST_LOCATION_S STUDY_VAL ("address");


STUDY_PREFIX const char *ST_CONTACT_S STUDY_VAL ("contact");

STUDY_PREFIX const char *ST_CURATOR_S STUDY_VAL ("curator");


STUDY_PREFIX const char *ST_CONTRIBUTORS_S STUDY_VAL ("contributors");


STUDY_PREFIX const char *ST_ASPECT_S STUDY_CONCAT_VAL (CONTEXT_PREFIX_NCI_THESAUSUS_ONTOLOGY_S, "C42677");

STUDY_PREFIX const char *ST_SLOPE_S STUDY_CONCAT_VAL (CONTEXT_PREFIX_ENVIRONMENT_ONTOLOGY_S, "00002000");


STUDY_PREFIX const char *ST_PARENT_FIELD_TRIAL_S STUDY_VAL ("parent_field_trial");

STUDY_PREFIX const char *ST_PLOTS_S STUDY_VAL ("plots");

STUDY_PREFIX const char *ST_NUMBER_OF_PLOTS_S STUDY_VAL ("number_of_plots");


STUDY_PREFIX const char *ST_DATA_LINK_S STUDY_CONCAT_VAL (CONTEXT_PREFIX_SCHEMA_ORG_S, "url");

STUDY_PREFIX const char *ST_FRICTIONLESS_DATA_LINK_S STUDY_CONCAT_VAL (CONTEXT_PREFIX_SCHEMA_ORG_S, "contentUrl");


STUDY_PREFIX const char *ST_HANDBOOK_DATA_LINK_S STUDY_VAL ("handbook");


STUDY_PREFIX const char *ST_CURRENT_CROP_S STUDY_VAL ("current_crop");

STUDY_PREFIX const char *ST_PREVIOUS_CROP_S STUDY_VAL ("previous_crop");

STUDY_PREFIX const char *ST_DESCRIPTION_S STUDY_CONCAT_VAL (CONTEXT_PREFIX_SCHEMA_ORG_S, "description");


STUDY_PREFIX const char *ST_DESIGN_S STUDY_VAL ("study_design");

STUDY_PREFIX const char *ST_GROWING_CONDITIONS_S STUDY_VAL ("growing_conditions");

STUDY_PREFIX const char *ST_PHENOTYPE_GATHERING_NOTES_S STUDY_VAL ("phenotype_gathering_notes");


STUDY_PREFIX const char *ST_NUMBER_OF_PLOT_ROWS_S STUDY_VAL ("num_rows");

STUDY_PREFIX const char *ST_NUMBER_OF_PLOT_COLUMN_S STUDY_VAL ("num_columns");

STUDY_PREFIX const char *ST_NUMBER_OF_REPLICATES_S STUDY_VAL ("num_replicates");


STUDY_PREFIX const char *ST_PLOT_LENGTH_S STUDY_VAL ("plot_length");

STUDY_PREFIX const char *ST_PLOT_WIDTH_S STUDY_VAL ("plot_width");


STUDY_PREFIX const char *ST_WEATHER_S STUDY_VAL ("weather");

STUDY_PREFIX const char *ST_SHAPE_S STUDY_VAL ("shape_data");

STUDY_PREFIX const char *ST_HAS_SHAPE_S STUDY_VAL ("has_shape_data");

STUDY_PREFIX const char *ST_SHAPE_NOTES_S STUDY_VAL ("shape_data_notes");


STUDY_PREFIX const char *ST_PLOT_H_GAP_S STUDY_VAL ("plot_horizontal_gap");

STUDY_PREFIX const char *ST_PLOT_V_GAP_S STUDY_VAL ("plot_vertical_gap");

STUDY_PREFIX const char *ST_PLOT_BLOCK_H_GAP_S STUDY_VAL ("plot_block_horizontal_gap");

STUDY_PREFIX const char *ST_PLOT_BLOCK_V_GAP_S STUDY_VAL ("plot_block_vertical_gap");

STUDY_PREFIX const char *ST_PLOT_ROWS_PER_BLOCK_S STUDY_VAL ("plot_block_rows");

STUDY_PREFIX const char *ST_PLOT_COLS_PER_BLOCK_S STUDY_VAL ("plot_block_columns");


STUDY_PREFIX const char *ST_TREATMENTS_S STUDY_VAL ("treatment_factors");


STUDY_PREFIX const char *ST_PHENOTYPES_S STUDY_VAL ("phenotypes");

STUDY_PREFIX const char *ST_ACCESSIONS_S STUDY_VAL ("accessions");


STUDY_PREFIX const char *ST_PHENOTYPE_STATISTICS_S STUDY_VAL ("statistics");

STUDY_PREFIX const char *ST_PHENOTYPE_DEFINITION_S STUDY_VAL ("definition");


STUDY_PREFIX const char *ST_HARVEST_YEAR_S STUDY_VAL ("harvest_year");

STUDY_PREFIX const char *ST_SOWING_YEAR_S STUDY_VAL ("sowing_year");


STUDY_PREFIX const char *ST_PLAN_CHANGES_S STUDY_VAL ("plan_changes");

STUDY_PREFIX const char *ST_PHYSICAL_SAMPLES_COLLECTED_S STUDY_VAL ("physical_samples_collected");

STUDY_PREFIX const char *ST_DATA_NOT_INCLUDED_S STUDY_VAL ("data_not_included");

STUDY_PREFIX const char *ST_PHOTO_URL_S STUDY_CONCAT_VAL (CONTEXT_PREFIX_SCHEMA_ORG_S, "image");

STUDY_PREFIX const char *ST_IMAGE_COLLECTION_NOTE_S STUDY_VAL ("image_collection_notes");

STUDY_PREFIX int32 ST_UNSET_PH STUDY_VAL (-1);


/**
 * The value to specify that the aspect parameter is not set.
 */
#define ST_UNKNOWN_DIRECTION_S "Unknown"




typedef struct Study
{
	bson_oid_t *st_id_p;

	Metadata *st_metadata_p;

	FieldTrial *st_parent_p;

	MEM_FLAG st_parent_field_trial_mem;

	struct Location *st_location_p;

	char *st_name_s;

	char *st_data_url_s;

	char *st_aspect_s;

	char *st_slope_s;

	/**
	 * A LinkedList of PlotNodes
	 * for all of the Plots in this
	 * Study.
	 */
	LinkedList *st_plots_p;

	Crop *st_current_crop_p;

	Crop *st_previous_crop_p;

	char *st_description_s;

	char *st_design_s;

	/**
	 * Changes to the Study field experiment plan.
	 */
	char *st_plan_changes_s;

	/**
	 * Details about plant, soil or other samples collected.
	 */
	char *st_physical_samples_collected_s;

	/**
	 * Data collected but not currently stored within Grassroots.
	 */
	char *st_data_not_included_s;

	char *st_growing_conditions_s;

	char *st_phenotype_gathering_notes_s;

	double64 *st_default_plot_width_p;

	double64 *st_default_plot_length_p;

	uint32 *st_num_rows_p;

	uint32 *st_num_columns_p;

	uint32 *st_num_replicates_p;

	char *st_weather_link_s;

	/**
	 * The GeoJSON data for the shape of the plots
	 */
	json_t *st_shape_p;

	/**
	 * Any notes about the GeoJSON data.
	 */
	char *st_shape_notes_s;

	double64 *st_plot_horizontal_gap_p;

	double64 *st_plot_vertical_gap_p;


	uint32 *st_plots_rows_per_block_p;

	uint32 *st_plots_columns_per_block_p;

	double64 *st_plot_block_horizontal_gap_p;

	double64 *st_plot_block_vertical_gap_p;

	LinkedList *st_treatments_p;

	Person *st_curator_p;

	Person *st_contact_p;

	uint32 *st_predicted_sowing_year_p;

	uint32 *st_predicted_harvest_year_p;

	char *st_photo_url_s;

	char *st_image_collection_notes_s;

	LinkedList *st_phenotypes_p;

	/**
	 * A LinkedList of PersonNodes of people who have contributed
	 * to this Study.
	 */
	LinkedList *st_contributors_p;



	/**
	 * A JSON object where the keys are the ObjectIDs of the
	 * materials within this Study and the values are how many
	 * times that they appear.
	 */
	json_t *st_accessions_p;

} Study;



typedef struct StudyNode
{
	ListItem stn_node;

	Study *stn_study_p;

} StudyNode;



/* forward declarations */
struct Plot;

#ifdef __cplusplus
extern "C"
{
#endif




DFW_FIELD_TRIAL_SERVICE_LOCAL Study *AllocateStudy (bson_oid_t *id_p,  Metadata *metadata_p, const char *name_s, const char *data_url_s, const char *aspect_s, const char *slope_s,
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


DFW_FIELD_TRIAL_SERVICE_LOCAL void FreeStudy (Study *study_p);


DFW_FIELD_TRIAL_SERVICE_LOCAL Study *CopyStudy (const Study * const src_p, const char * const new_name_s, const bool copy_treatment_factors_flag, const bool copy_measured_variables_flag, const FieldTrialServiceData *data_p);


DFW_FIELD_TRIAL_SERVICE_LOCAL StudyNode *AllocateStudyNode (Study *study_p);

DFW_FIELD_TRIAL_SERVICE_LOCAL void FreeStudyNode (ListItem *node_p);

DFW_FIELD_TRIAL_SERVICE_LOCAL json_t *GetStudyAsJSON (Study *study_p, const ViewFormat format, JSONProcessor *processor_p, FieldTrialServiceData *data_p);

DFW_FIELD_TRIAL_SERVICE_LOCAL Study *GetStudyFromJSON (const json_t *json_p, const ViewFormat format, const FieldTrialServiceData *data_p);

DFW_FIELD_TRIAL_SERVICE_LOCAL Study *GetStudyWithParentTrialFromJSON (const json_t *json_p, FieldTrial *parent_trial_p, const ViewFormat format, const FieldTrialServiceData *data_p);

DFW_FIELD_TRIAL_SERVICE_LOCAL bool GetStudyPlots (Study *study_p, const ViewFormat format, FieldTrialServiceData *data_p);

DFW_FIELD_TRIAL_SERVICE_LOCAL OperationStatus SaveStudy (Study *study_p, ServiceJob *job_p, FieldTrialServiceData *data_p, const char *url_key_s);

DFW_FIELD_TRIAL_SERVICE_LOCAL Study *GetStudyByIdString (const char *study_id_s, const ViewFormat format, const FieldTrialServiceData *data_p);

DFW_FIELD_TRIAL_SERVICE_LOCAL Study *GetStudyById (bson_oid_t *st_id_p, const ViewFormat format, const FieldTrialServiceData *data_p);

DFW_FIELD_TRIAL_SERVICE_LOCAL bool HasStudyGotPlotLayoutDetails (const Study *study_p);

DFW_FIELD_TRIAL_SERVICE_LOCAL bool AddPhenotypesToJSON (const Study *study_p, json_t *study_json_p, const ViewFormat format, const FieldTrialServiceData *data_p);


DFW_FIELD_TRIAL_SERVICE_LOCAL bool AddPlotToStudy (Study *study_p, struct Plot *plot_p);


DFW_FIELD_TRIAL_SERVICE_LOCAL int64 GetNumberOfPlotsInStudy (const Study *study_p, const FieldTrialServiceData *data_p);

DFW_FIELD_TRIAL_SERVICE_LOCAL OperationStatus IndexStudy (Study *study_p, ServiceJob *job_p, const char *job_name_s, FieldTrialServiceData *data_p);


DFW_FIELD_TRIAL_SERVICE_LOCAL bool AddStudyContributor (Study *study_p, Person *person_p, MEM_FLAG mf);


DFW_FIELD_TRIAL_SERVICE_LOCAL bool IsMeasuredVariableOnStudy (const Study * const study_p, const char *mv_s);


DFW_FIELD_TRIAL_SERVICE_LOCAL bool AddPhenotypeStatisticsToStudy (Study * const study_p, const char *mv_s, const Statistics *stats_p);


DFW_FIELD_TRIAL_SERVICE_LOCAL bool CopyAllPhenotypeStatistics (const Study * const src_p, Study *dest_p, const bool remove_existing_entries_flag);


DFW_FIELD_TRIAL_SERVICE_LOCAL bool SetStudyAccessions (Study * const study_p, json_t *accessions_p);



#ifdef __cplusplus
}
#endif


#endif /* SERVICES_FIELD_TRIALS_INCLUDE_STUDY_H_ */
