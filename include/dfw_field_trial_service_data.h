/*
** Copyright 2014-2016 The Earlham Institute
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

/**
 * @file
 * @brief
 */
/*
 * dfw_field_trial_service_data.h
 *
 *  Created on: 8 Jul 2018
 *      Author: tyrrells
 */

#ifndef DFW_FIELD_TRIAL_SERVICE_DATA_H_
#define DFW_FIELD_TRIAL_SERVICE_DATA_H_

#include "dfw_field_trial_service_library.h"
#include "jansson.h"

#include "service.h"
#include "mongodb_tool.h"
//#include "sqlite_tool.h"
#include "view_format.h"


typedef enum
{
	DFTD_PROGRAMME,
	DFTD_FIELD_TRIAL,
	DFTD_STUDY,
	DFTD_LOCATION,
	DFTD_PLOT,
/*	DFTD_ROW, */
	DFTD_MATERIAL,
	DFTD_DRILLING,
	DFTD_MEASURED_VARIABLE,
	DFTD_OBSERVATION,
	DFTD_INSTRUMENT,
	DFTD_GENE_BANK,
	DFTD_CROP,
	DFTD_TREATMENT,
	DFTD_ONTOLOGY,
	DFTD_NUM_TYPES
} FieldTrialDatatype;





/*
	Crop Ontology Scale Class types

1. Date
The date class is for events expressed in a time format, e.g. “yyyymmdd hh:mm:ss –
UTC” or “dd-mm-yy”. A good practice recommended by the Breeding API (BrAPI) is to
use the Date and timestamp fields coded in the ISO 8601 standard, extended format.
Check
https://github.com/plantbreeding/API/blob/master/Specification/GeneralInfo/Date_Time
_Encoding.md)

2. Duration
The duration class is for time elapsed between two events expressed in a time format,
e.g. “days”, “hours”, “months”.

3. Nominal
Categorical scale that can take one of a limited number of categories. There is no
intrinsic ordering to the categories e.g. r=“red”, g=“green”, p=“purple”.

4. Numerical
Numerical scales express the trait with real numbers. The numerical scale defines the
unit e.g. centimetre, ton per hectare, number of branches.

5. Ordinal
Ordinal scales are composed of ordered and fixed number of categories e.g. 1=low,
2=moderate, 3=high

6. Text
A free text is used to express the scale value. Also known as Character variable
(varchar)
e.g. “Preferred when slightly undercooked”.

7. Code
This scale class is exceptionally used to express complex traits. Code is a nominal
scale that combines the expressions of the different traits composing the complex trait.
For example, a disease related code might be expressed by a 2-digit code for intensity
and 2-character code for severity. The first 2 digits are the proportion of plants affected
by a fungus and the 2 characters refer to the severity, e.g. “75HD” means “75% of the
plants are infected and plants are highly damaged”. It is recommended to create
variables for every component of the code.
*/

typedef struct ScaleClass
{
	const char *sc_name_s;
	ParameterType sc_type;
} ScaleClass;


/**
 * The configuration data used by the DFW Field Trial Service.
 *
 * @extends ServiceData
 * @ingroup field_trials_service
 */
typedef struct /*DFW_FIELD_TRIAL_SERVICE_LOCAL*/ FieldTrialServiceData
{
	/** The base ServiceData. */
	ServiceData dftsd_base_data;


	/**
	 * @private
	 *
	 * The MongoTool to connect to the database where our data is stored.
	 */
//	MongoTool *dftsd_mongo_p;


	/**
	 * @private
	 *
	 * The name of the database to use.
	 */
	const char *dftsd_database_s;

	/**
	 * @private
	 *
	 * The collection name of use for each of the different types of data.
	 */
	const char *dftsd_collection_ss [DFTD_NUM_TYPES];

	/**
	 * @private
	 *
	 * The names of the collections to use to store the historical versions for the data
	 * for each of the different types of objects.
	 */
	const char *dftsd_backup_collection_ss [DFTD_NUM_TYPES];


	/**
	 * @private
	 *
	 * The key used for facets in Lucene
	 */
	const char *dftsd_facet_key_s;


	/**
	 * @private
	 *
	 * The path to where cached studies are saved.
	 */
	const char *dftsd_study_cache_path_s;


	/**
	 * @private
	 *
	 * The filesystem path to where to save the Studies will be backed up to
	 * when they are deleted
	 */
	const char *dftsd_wastebasket_path_s;


	/**
	 * @private
	 *
	 * The filesystem path to where to save the uploaded plot submission data to
	 */
	const char *dftsd_plots_uploads_path_s;

	/**
	 * @private
	 *
	 * The filesystem path to where to save the Frictionless Data Packages,
	 * Latex and PDF files for the Studies.
	 */
	const char *dftsd_assets_path_s;

	/**
	 * @private
	 *
	 * The base url to where the Frictionless Data Packages
	 * for the Studies will be served from.
	 */
	const char *dftsd_fd_url_s;


	/**
	 * @private
	 *
	 * The base url for where each study can be viewed on the
	 * front end.
	 */
	const char *dftsd_view_study_url_s;


	/**
	 * @private
	 *
	 * The base url for where each trial can be viewed on the
	 * front end.
	 */
	const char *dftsd_view_trial_url_s;

	/**
	 * @private
	 *
	 * The base url for where each location can be viewed on the
	 * front end.
	 */
	const char *dftsd_view_location_url_s;


	/**
	 * @private
	 *
	 * The base url for where each programme can be viewed on the
	 * front end.
	 */
	const char *dftsd_view_programme_url_s;


	/**
	 * @private
	 *
	 * The base url for where each study's plots can be viewed on the
	 * front end.
	 */
	const char *dftsd_view_plots_url_s;




	/**
	 * @private
	 *
	 * The command for running pdflatex.
	 */
	const char *dftsd_latex_commmand_s;


	LinkedList *dftsd_measured_variables_cache_p;


	LinkedList *dftsd_treatments_cache_p;


	/**
	 * @private
	 *
	 * The GeoAPIFY api key for generating static
	 * map images for the handbook
	 */
	const char *dftsd_geoapify_key_s;

	const char *dftsd_map_tile_width_s;


	const char *dftsd_map_tile_height_s;


	/**
	 * @private
	 *
	 * The filesystem path to where any phenotype heatmap images
	 * are stored.
	 */
	const char *dftsd_phenotype_images_path_s;

	const char *dftsd_marti_api_url_s;

	const char *dftsd_grassroots_marti_search_url_s;


} FieldTrialServiceData;



#ifndef DOXYGEN_SHOULD_SKIP_THIS

#ifdef ALLOCATE_DFW_FIELD_TRIAL_SERVICE_TAGS
	#define DFW_FIELD_TRIAL_PREFIX DFW_FIELD_TRIAL_SERVICE_API
	#define DFW_FIELD_TRIAL_VAL(x)	= x
	#define DFW_FIELD_TRIAL_STRUCT_VAL(x,y)	= { x, y }
	#define DFW_FIELD_TRIAL_CONCAT_VAL(x,y) = x y
#else
	#define DFW_FIELD_TRIAL_PREFIX extern
	#define DFW_FIELD_TRIAL_VAL(x)
	#define DFW_FIELD_TRIAL_STRUCT_VAL(x,y)
	#define DFW_FIELD_TRIAL_CONCAT_VAL(x,y) = x y

#endif

#endif 		/* #ifndef DOXYGEN_SHOULD_SKIP_THIS */



/**
 * The key for specifying the object containing the fields data
 *
 * @ingroup field_trials_service
 */
DFW_FIELD_TRIAL_PREFIX const char *DFT_PROGRAM_S DFW_FIELD_TRIAL_VAL ("Programs");

DFW_FIELD_TRIAL_PREFIX const char *DFT_PROGRAM_BACKUP_S DFW_FIELD_TRIAL_VAL ("Programs_versions");


/**
 * The key for specifying the object containing the fields data
 *
 * @ingroup field_trials_service
 */
DFW_FIELD_TRIAL_PREFIX const char *DFT_FIELD_TRIALS_S DFW_FIELD_TRIAL_VAL ("FieldTrials");

DFW_FIELD_TRIAL_PREFIX const char *DFT_FIELD_TRIALS_BACKUP_S DFW_FIELD_TRIAL_VAL ("FieldTrials_versions");

/**
 * The key for specifying the object containing the experimental area data
 *
 * @ingroup field_trials_service
 */
DFW_FIELD_TRIAL_PREFIX const char *DFT_STUDIES_S DFW_FIELD_TRIAL_VAL ("Studies");

DFW_FIELD_TRIAL_PREFIX const char *DFT_STUDIES_BACKUP_S DFW_FIELD_TRIAL_VAL ("Studies_versions");

/**
 * The key for specifying the object containing the location data
 *
 * @ingroup field_trials_service
 */
DFW_FIELD_TRIAL_PREFIX const char *DFT_LOCATION_S DFW_FIELD_TRIAL_VAL ("Locations");

DFW_FIELD_TRIAL_PREFIX const char *DFT_LOCATION_BACKUP_S DFW_FIELD_TRIAL_VAL ("Locations_versions");

/**
 * The key for specifying the object containing the plot data
 *
 * @ingroup field_trials_service
 */
DFW_FIELD_TRIAL_PREFIX const char *DFT_PLOT_S DFW_FIELD_TRIAL_VAL ("Plots");

DFW_FIELD_TRIAL_PREFIX const char *DFT_PLOT_BACKUP_S DFW_FIELD_TRIAL_VAL ("Plots_versions");

/**
 * The key for specifying the object containing the drilling data
 *
 * @ingroup field_trials_service
 */
DFW_FIELD_TRIAL_PREFIX const char *DFT_DRILLING_S DFW_FIELD_TRIAL_VAL ("Drillings");

DFW_FIELD_TRIAL_PREFIX const char *DFT_DRILLING_BACKUP_S DFW_FIELD_TRIAL_VAL ("Drillings_versions");


/**
 * The key for specifying the object containing the materials data
 *
 * @ingroup field_trials_service
 */
DFW_FIELD_TRIAL_PREFIX const char *DFT_MATERIAL_S DFW_FIELD_TRIAL_VAL ("Materials");

DFW_FIELD_TRIAL_PREFIX const char *DFT_MATERIAL_BACKUP_S DFW_FIELD_TRIAL_VAL ("Materials_versions");

/**
 * The key for specifying the object containing the  phenotype data
 *
 * @ingroup field_trials_service
 */
DFW_FIELD_TRIAL_PREFIX const char *DFT_PHENOTYPE_S DFW_FIELD_TRIAL_VAL ("Phenotypes");

DFW_FIELD_TRIAL_PREFIX const char *DFT_PHENOTYPE_BACKUP_S DFW_FIELD_TRIAL_VAL ("Phenotypes_versions");

/**
 * The key for specifying the object containing the observation data
 *
 * @ingroup field_trials_service
 */
DFW_FIELD_TRIAL_PREFIX const char *DFT_OBSERVATION_S DFW_FIELD_TRIAL_VAL ("Observations");

DFW_FIELD_TRIAL_PREFIX const char *DFT_OBSERVATION_BACKUP_S DFW_FIELD_TRIAL_VAL ("Observations_versions");

/**
 * The key for specifying the object containing the instruments
 *
 * @ingroup field_trials_service
 */
DFW_FIELD_TRIAL_PREFIX const char *DFT_INSTRUMENT_S DFW_FIELD_TRIAL_VAL ("Instruments");

DFW_FIELD_TRIAL_PREFIX const char *DFT_INSTRUMENT_BACKUP_S DFW_FIELD_TRIAL_VAL ("Instruments_versions");

/**
 * The key for specifying the object containing the Gene Banks
 *
 * @ingroup field_trials_service
 */
DFW_FIELD_TRIAL_PREFIX const char *DFT_GENE_BANK_S DFW_FIELD_TRIAL_VAL ("GeneBanks");

DFW_FIELD_TRIAL_PREFIX const char *DFT_GENE_BANK_BACKUP_S DFW_FIELD_TRIAL_VAL ("GeneBanks_versions");


/**
 * The key for specifying the object containing the rows within the plots.
 *
 * @ingroup field_trials_service
 */
//DFW_FIELD_TRIAL_PREFIX const char *DFT_ROW_S DFW_FIELD_TRIAL_VAL ("Rows");


/**
 * The key for specifying the object containing the crops.
 *
 * @ingroup field_trials_service
 */
DFW_FIELD_TRIAL_PREFIX const char *DFT_CROP_S DFW_FIELD_TRIAL_VAL ("Crops");

DFW_FIELD_TRIAL_PREFIX const char *DFT_CROP_BACKUP_S DFW_FIELD_TRIAL_VAL ("Crops_versions");

/**
 * The key for specifying the object containing the treatments.
 *
 * @ingroup field_trials_service
 */
DFW_FIELD_TRIAL_PREFIX const char *DFT_TREATMENT_S DFW_FIELD_TRIAL_VAL ("Treatments");

DFW_FIELD_TRIAL_PREFIX const char *DFT_TREATMENT_BACKUP_S DFW_FIELD_TRIAL_VAL ("Treatments_versions");



/**
 * The key for specifying the object containing the phenotype ontologies.
 *
 * @ingroup field_trials_service
 */
DFW_FIELD_TRIAL_PREFIX const char *DFT_PHENOTYPE_ONOTOLOGY_S DFW_FIELD_TRIAL_VAL ("Phenotype_Ontologies");

DFW_FIELD_TRIAL_PREFIX const char *DFT_PHENOTYPE_ONOTOLOGY_BACKUP_S DFW_FIELD_TRIAL_VAL ("Phenotype_Ontologies_versions");



/**
 * The key for specifying whether a particular object in a JSON tree is
 * selected, e.g. matched a search.
 *
 * @ingroup field_trials_service
 */
DFW_FIELD_TRIAL_PREFIX const char *DFT_SELECTED_S DFW_FIELD_TRIAL_VAL ("selected");



DFW_FIELD_TRIAL_PREFIX const char DFT_DEFAULT_COLUMN_DELIMITER DFW_FIELD_TRIAL_VAL (',');

DFW_FIELD_TRIAL_PREFIX const char *DFT_BACKUPS_ID_KEY_S DFW_FIELD_TRIAL_VAL ("original_id");


/** The prefix to use for Field Trial Service aliases. */
#define DFT_GROUP_ALIAS_PREFIX_S "field_trial"



DFW_FIELD_TRIAL_PREFIX const ScaleClass SCALE_DURATION DFW_FIELD_TRIAL_STRUCT_VAL("Duration", PT_SIGNED_REAL);
DFW_FIELD_TRIAL_PREFIX const ScaleClass SCALE_NOMINAL DFW_FIELD_TRIAL_STRUCT_VAL ("Nominal", PT_STRING);
DFW_FIELD_TRIAL_PREFIX const ScaleClass SCALE_NUMERICAL DFW_FIELD_TRIAL_STRUCT_VAL ("Numerical", PT_SIGNED_REAL);
DFW_FIELD_TRIAL_PREFIX const ScaleClass SCALE_CODE DFW_FIELD_TRIAL_STRUCT_VAL ("Code", PT_STRING);
DFW_FIELD_TRIAL_PREFIX const ScaleClass SCALE_ORDINAL DFW_FIELD_TRIAL_STRUCT_VAL ("Ordinal", PT_UNSIGNED_INT);
DFW_FIELD_TRIAL_PREFIX const ScaleClass SCALE_TEXT DFW_FIELD_TRIAL_STRUCT_VAL ("Text", PT_STRING);
DFW_FIELD_TRIAL_PREFIX const ScaleClass SCALE_DATE DFW_FIELD_TRIAL_STRUCT_VAL ("Date", PT_TIME);


/*
 * forward declarations
 */
struct MeasuredVariable;
struct Treatment;

#ifdef __cplusplus
extern "C"
{
#endif

DFW_FIELD_TRIAL_SERVICE_LOCAL FieldTrialServiceData *AllocateFieldTrialServiceData (void);


DFW_FIELD_TRIAL_SERVICE_LOCAL void FreeFieldTrialServiceData (FieldTrialServiceData *data_p);


DFW_FIELD_TRIAL_SERVICE_LOCAL bool ConfigureFieldTrialService (FieldTrialServiceData *data_p, GrassrootsServer *grassroots_p);


DFW_FIELD_TRIAL_SERVICE_LOCAL const char *GetDatatypeAsString (const FieldTrialDatatype data_type);


DFW_FIELD_TRIAL_SERVICE_LOCAL const char *GetDatatypeDescriptionAsString (const FieldTrialDatatype data_type);


DFW_FIELD_TRIAL_SERVICE_LOCAL FieldTrialDatatype GetDatatypeFromString (const char *type_s);


DFW_FIELD_TRIAL_SERVICE_LOCAL const char *GetImageForDatatype (const FieldTrialServiceData *data_p, const char *data_type_s);


DFW_FIELD_TRIAL_SERVICE_LOCAL bool EnableMeasuredVariablesCache (FieldTrialServiceData *data_p);


DFW_FIELD_TRIAL_SERVICE_LOCAL void ClearMeasuredVariablesCache (FieldTrialServiceData *data_p);


DFW_FIELD_TRIAL_SERVICE_LOCAL struct MeasuredVariable *GetCachedMeasuredVariableById (FieldTrialServiceData *data_p, const char *mv_id_s);


DFW_FIELD_TRIAL_SERVICE_LOCAL struct MeasuredVariable *GetCachedMeasuredVariableByName (FieldTrialServiceData *data_p, const char *name_s);


DFW_FIELD_TRIAL_SERVICE_LOCAL bool AddMeasuredVariableToCache (FieldTrialServiceData *data_p, struct MeasuredVariable *mv_p, MEM_FLAG mf);


DFW_FIELD_TRIAL_SERVICE_LOCAL bool RemoveCachedMeasuredVariableByName (FieldTrialServiceData *data_p, const char *name_s);


DFW_FIELD_TRIAL_SERVICE_LOCAL bool HasMeasuredVariableCache (FieldTrialServiceData *data_p);


DFW_FIELD_TRIAL_SERVICE_LOCAL bool EnableTreatmentsCache (FieldTrialServiceData *data_p);


DFW_FIELD_TRIAL_SERVICE_LOCAL void ClearTreatmentsCache (FieldTrialServiceData *data_p);


DFW_FIELD_TRIAL_SERVICE_LOCAL struct Treatment *GetCachedTreatmentByURL (FieldTrialServiceData *data_p, const char *url_s);


DFW_FIELD_TRIAL_SERVICE_LOCAL bool AddTreatmentToCache (FieldTrialServiceData *data_p, struct Treatment *treatment_p, MEM_FLAG mf);


DFW_FIELD_TRIAL_SERVICE_LOCAL bool HasTreatmentCache (FieldTrialServiceData *data_p);


DFW_FIELD_TRIAL_SERVICE_LOCAL char *GetFullCacheFilename (const char *name_s, const FieldTrialServiceData *data_p);


DFW_FIELD_TRIAL_SERVICE_LOCAL MongoTool *GetConfiguredMongoTool (const FieldTrialServiceData *data_p, GrassrootsServer *grassroots_p);



#ifdef __cplusplus
}
#endif


#endif /* DFW_FIELD_TRIAL_SERVICE_DATA_H_ */
