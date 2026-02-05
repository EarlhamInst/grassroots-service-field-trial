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
 * dfw_field_trial_service_data.c
 *
 *  Created on: 18 Sep 2018
 *      Author: billy
 */

#define ALLOCATE_DFW_FIELD_TRIAL_SERVICE_TAGS (1)
#include "dfw_field_trial_service_data.h"

#include "measured_variable.h"
#include "treatment.h"

#include "jansson.h"

#include "streams.h"
#include "string_utils.h"


static const char *S_TYPES_SS [DFTD_NUM_TYPES] =
{
	"Grassroots:Programme",
	"Grassroots:FieldTrial",
	"Grassroots:Study",
	"Grassroots:Location",
	"Grassroots:Plot",
/*	"Grassroots:Row", */
 	"Grassroots:Material",
	"Grassroots:Drilling",
	"Grassroots:MeasuredVariable",
	"Grassroots:Observation",
	"Grassroots:Instrument",
	"Grassroots:GeneBank",
	"Grassroots:Crop",
	"Grassroots:Treatment",
	"Grassroots:Ontology"
};


static const char *S_TYPE_DESCRIPTIONS_SS [DFTD_NUM_TYPES] =
{
	"Programme",
	"Field Trial",
	"Study",
	"Location",
	"Plot",
/*	"Grassroots:Row", */
 	"Material",
	"Drilling",
	"Measured Variable",
	"Observation",
	"Instrument",
	"Gene Bank",
	"Crop",
	"Treatment",
	"Ontology"
};


static MeasuredVariableNode *GetCachedMeasuredVariableNodeByName (const FieldTrialServiceData *data_p, const char *name_s);



FieldTrialServiceData *AllocateFieldTrialServiceData (void)
{
	FieldTrialServiceData *data_p = (FieldTrialServiceData *) AllocMemory (sizeof (FieldTrialServiceData));

	if (data_p)
		{
			data_p -> dftsd_database_s = NULL;
			data_p -> dftsd_facet_key_s = NULL;
			data_p -> dftsd_study_cache_path_s = NULL;
			data_p -> dftsd_wastebasket_path_s = NULL;

			data_p -> dftsd_plots_uploads_path_s = NULL;

			data_p -> dftsd_measured_variables_cache_p = NULL;

			data_p -> dftsd_view_study_url_s = NULL;
			data_p -> dftsd_view_trial_url_s = NULL;
			data_p -> dftsd_view_plots_url_s = NULL;
			data_p -> dftsd_view_location_url_s = NULL;
			data_p -> dftsd_view_programme_url_s = NULL;

			data_p -> dftsd_latex_commmand_s = NULL;

			data_p -> dftsd_treatments_cache_p = NULL;

			data_p -> dftsd_assets_path_s = NULL;

			data_p -> dftsd_fd_url_s = NULL;

			data_p -> dftsd_geoapify_key_s = NULL;

			data_p -> dftsd_map_tile_width_s = NULL;

			data_p -> dftsd_map_tile_height_s = NULL;

			data_p -> dftsd_phenotype_images_path_s = NULL;

			memset (data_p -> dftsd_collection_ss, 0, DFTD_NUM_TYPES * sizeof (const char *));

			data_p -> dftsd_marti_api_url_s = NULL;

			data_p -> dftsd_grassroots_marti_search_url_s = NULL;

			return data_p;
		}

	return NULL;
}


bool EnableMeasuredVariablesCache (FieldTrialServiceData *data_p)
{
	bool success_flag = true;

	if (! (data_p -> dftsd_measured_variables_cache_p))
		{
			data_p -> dftsd_measured_variables_cache_p = AllocateLinkedList (FreeMeasuredVariableNode);

			if (! (data_p -> dftsd_measured_variables_cache_p))
				{
					success_flag = false;
				}
		}

	return success_flag;
}


void ClearMeasuredVariablesCache (FieldTrialServiceData *data_p)
{
	if (data_p -> dftsd_measured_variables_cache_p)
		{
			ClearLinkedList (data_p -> dftsd_measured_variables_cache_p);
		}
}



bool EnableTreatmentsCache (FieldTrialServiceData *data_p)
{
	bool success_flag = true;

	if (! (data_p -> dftsd_treatments_cache_p))
		{
			data_p -> dftsd_treatments_cache_p = AllocateLinkedList (FreeTreatmentNode);

			if (! (data_p -> dftsd_treatments_cache_p))
				{
					success_flag = false;
				}
		}

	return success_flag;
}


void ClearTreatmentsCache (FieldTrialServiceData *data_p)
{
	if (data_p -> dftsd_treatments_cache_p)
		{
			ClearLinkedList (data_p -> dftsd_treatments_cache_p);
		}
}



void FreeFieldTrialServiceData (FieldTrialServiceData *data_p)
{
	if (data_p -> dftsd_measured_variables_cache_p)
		{
			FreeLinkedList (data_p -> dftsd_measured_variables_cache_p);
		}

	if (data_p -> dftsd_treatments_cache_p)
		{
			FreeLinkedList (data_p -> dftsd_treatments_cache_p);
		}


	FreeMemory (data_p);
}


MongoTool *GetConfiguredMongoTool (const FieldTrialServiceData *data_p, GrassrootsServer *grassroots_p)
{
	if (data_p -> dftsd_database_s)
		{
			if (!grassroots_p)
				{
					if (data_p -> dftsd_base_data.sd_service_p)
						{
							grassroots_p = data_p -> dftsd_base_data.sd_service_p -> se_grassroots_p;
						}
				}

			if (grassroots_p)
				{
					MongoTool *mongo_p = AllocateMongoTool (NULL, grassroots_p -> gs_mongo_manager_p);

					if (mongo_p)
						{
							if (SetMongoToolDatabase (mongo_p, data_p -> dftsd_database_s))
								{
									return mongo_p;
								}
							else
								{
									PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to set db to \"%s\"", data_p -> dftsd_database_s);
								}

							FreeMongoTool (mongo_p);
						}
				}
		}

	return NULL;
}


bool ConfigureFieldTrialService (FieldTrialServiceData *data_p, GrassrootsServer *grassroots_p)
{
	bool success_flag = false;
	const json_t *service_config_p = data_p -> dftsd_base_data.sd_config_p;

	data_p -> dftsd_database_s = GetJSONString (service_config_p, "database");

	if (data_p -> dftsd_database_s)
		{
			bool enable_db_cache_flag = false;
			const char * const BACKUP_SUFFIX_S = "_backup";
			success_flag = true;

			data_p -> dftsd_study_cache_path_s = GetJSONString (service_config_p, "cache_path");

			if (data_p -> dftsd_study_cache_path_s)
				{
					if (!EnsureDirectoryExists (data_p -> dftsd_study_cache_path_s))
						{
							PrintErrors (STM_LEVEL_WARNING, __FILE__, __LINE__, "Failed to create studies cache directory \"%s\"", data_p -> dftsd_study_cache_path_s);
							data_p -> dftsd_study_cache_path_s = NULL;
						}
				}

			data_p -> dftsd_assets_path_s = GetJSONString (service_config_p, "fd_path");

			if (data_p -> dftsd_assets_path_s)
				{
					if (!EnsureDirectoryExists (data_p -> dftsd_assets_path_s))
						{
							PrintErrors (STM_LEVEL_WARNING, __FILE__, __LINE__, "Failed to create frictionless data packages directory \"%s\"", data_p -> dftsd_assets_path_s);
							data_p -> dftsd_assets_path_s = NULL;
						}
				}

			data_p -> dftsd_fd_url_s = GetJSONString (service_config_p, "fd_url");

			data_p -> dftsd_view_study_url_s = GetJSONString (service_config_p, "view_study_url");
			data_p -> dftsd_view_trial_url_s = GetJSONString (service_config_p, "view_trial_url");
			data_p -> dftsd_view_location_url_s = GetJSONString (service_config_p, "view_location_url");
			data_p -> dftsd_view_programme_url_s = GetJSONString (service_config_p, "view_programme_url");
			data_p -> dftsd_view_plots_url_s = GetJSONString (service_config_p, "view_plots_url");

			data_p -> dftsd_latex_commmand_s = GetJSONString (service_config_p, "pdflatex_path");
			if (! (data_p -> dftsd_latex_commmand_s))
				{
					data_p -> dftsd_latex_commmand_s = "pdflatex";
				}


			data_p -> dftsd_wastebasket_path_s = GetJSONString (service_config_p, "wastebasket_path");

			data_p -> dftsd_plots_uploads_path_s = GetJSONString (service_config_p, "plots_uploads_path");


			data_p -> dftsd_geoapify_key_s = GetJSONString (service_config_p, "geoapify_api_key");

			data_p -> dftsd_map_tile_width_s = GetJSONString (service_config_p, "map_tile_width");
			data_p -> dftsd_map_tile_height_s = GetJSONString (service_config_p, "map_tile_height");


			data_p -> dftsd_phenotype_images_path_s = GetJSONString (service_config_p, "handbook_phenotype_images");


			data_p -> dftsd_marti_api_url_s = GetJSONString (service_config_p, "marti_api_url");

			data_p -> dftsd_grassroots_marti_search_url_s = GetJSONString (service_config_p, "grassroots_marti_service_url");


			* ((data_p -> dftsd_collection_ss) + DFTD_PROGRAMME) = DFT_PROGRAM_S;
			* ((data_p -> dftsd_collection_ss) + DFTD_FIELD_TRIAL) = DFT_FIELD_TRIALS_S;
			* ((data_p -> dftsd_collection_ss) + DFTD_STUDY) = DFT_STUDIES_S;
			* ((data_p -> dftsd_collection_ss) + DFTD_LOCATION) = DFT_LOCATION_S;
			* ((data_p -> dftsd_collection_ss) + DFTD_PLOT) = DFT_PLOT_S;
			* ((data_p -> dftsd_collection_ss) + DFTD_MATERIAL) = DFT_MATERIAL_S;
			* ((data_p -> dftsd_collection_ss) + DFTD_DRILLING) = DFT_DRILLING_S;
			* ((data_p -> dftsd_collection_ss) + DFTD_MEASURED_VARIABLE) = DFT_PHENOTYPE_S;
			* ((data_p -> dftsd_collection_ss) + DFTD_OBSERVATION) = DFT_OBSERVATION_S;
			* ((data_p -> dftsd_collection_ss) + DFTD_INSTRUMENT) = DFT_INSTRUMENT_S;
			* ((data_p -> dftsd_collection_ss) + DFTD_GENE_BANK) = DFT_GENE_BANK_S;
			// * ((data_p -> dftsd_collection_ss) + DFTD_ROW) = DFT_ROW_S;
			* ((data_p -> dftsd_collection_ss) + DFTD_CROP) = DFT_CROP_S;
			* ((data_p -> dftsd_collection_ss) + DFTD_TREATMENT) = DFT_TREATMENT_S;
			* ((data_p -> dftsd_collection_ss) + DFTD_ONTOLOGY) = DFT_PHENOTYPE_ONOTOLOGY_S;

			* ((data_p -> dftsd_backup_collection_ss) + DFTD_PROGRAMME) = DFT_PROGRAM_BACKUP_S;
			* ((data_p -> dftsd_backup_collection_ss) + DFTD_FIELD_TRIAL) = DFT_FIELD_TRIALS_BACKUP_S;
			* ((data_p -> dftsd_backup_collection_ss) + DFTD_STUDY) = DFT_STUDIES_BACKUP_S;
			* ((data_p -> dftsd_backup_collection_ss) + DFTD_LOCATION) = DFT_LOCATION_BACKUP_S;
			* ((data_p -> dftsd_backup_collection_ss) + DFTD_PLOT) = DFT_PLOT_BACKUP_S;
			* ((data_p -> dftsd_backup_collection_ss) + DFTD_MATERIAL) = DFT_MATERIAL_BACKUP_S;
			* ((data_p -> dftsd_backup_collection_ss) + DFTD_DRILLING) = DFT_DRILLING_BACKUP_S;
			* ((data_p -> dftsd_backup_collection_ss) + DFTD_MEASURED_VARIABLE) = DFT_PHENOTYPE_BACKUP_S;
			* ((data_p -> dftsd_backup_collection_ss) + DFTD_OBSERVATION) = DFT_OBSERVATION_BACKUP_S;
			* ((data_p -> dftsd_backup_collection_ss) + DFTD_INSTRUMENT) = DFT_INSTRUMENT_BACKUP_S;
			* ((data_p -> dftsd_backup_collection_ss) + DFTD_GENE_BANK) = DFT_GENE_BANK_BACKUP_S;
			// * ((data_p -> dftsd_collection_ss) + DFTD_ROW) = DFT_ROW_S;
			* ((data_p -> dftsd_backup_collection_ss) + DFTD_CROP) = DFT_CROP_S;
			* ((data_p -> dftsd_backup_collection_ss) + DFTD_TREATMENT) = DFT_TREATMENT_S;
			* ((data_p -> dftsd_backup_collection_ss) + DFTD_ONTOLOGY) = DFT_PHENOTYPE_ONOTOLOGY_S;



			GetJSONBoolean (service_config_p, "use_mv_cache", &enable_db_cache_flag);

			if (enable_db_cache_flag)
				{
					if (!EnableMeasuredVariablesCache (data_p))
						{
							PrintErrors (STM_LEVEL_WARNING, __FILE__, __LINE__, "Failed to enable measured variable cache");
						}
				}

			GetJSONBoolean (service_config_p, "use_treatments_cache", &enable_db_cache_flag);

			if (enable_db_cache_flag)
				{
					if (!EnableTreatmentsCache (data_p))
						{
							PrintErrors (STM_LEVEL_WARNING, __FILE__, __LINE__, "Failed to enable treatmnets cache");
						}
				}

		} /* if (data_p -> psd_database_s) */

	return success_flag;
}



const char *GetDatatypeAsString (const FieldTrialDatatype data_type)
{
	const char *type_s = NULL;

	if (data_type < DFTD_NUM_TYPES)
		{
			type_s =  * (S_TYPES_SS + data_type);
		}

	return type_s;
}


const char *GetDatatypeDescriptionAsString (const FieldTrialDatatype data_type)
{
	const char *type_s = NULL;

	if (data_type < DFTD_NUM_TYPES)
		{
			type_s =  * (S_TYPE_DESCRIPTIONS_SS + data_type);
		}

	return type_s;
}


FieldTrialDatatype GetDatatypeFromString (const char *type_s)
{
	if (type_s)
		{
			FieldTrialDatatype i = 0;

			for ( ; i < DFTD_NUM_TYPES; ++ i)
				{
					if (strcmp (* (S_TYPES_SS + i), type_s) == 0)
						{
							return i;
						}
				}

		}		/* if (type_s) */

	return DFTD_NUM_TYPES;
}



const char *GetImageForDatatype (const FieldTrialServiceData *data_p, const char *data_type_s)
{
	const char *image_s = NULL;
	const json_t *images_config_p = json_object_get (data_p -> dftsd_base_data.sd_config_p, "images");

	if (images_config_p)
		{
			image_s = GetJSONString (images_config_p, data_type_s);
		}


	return image_s;
}


MeasuredVariable *GetCachedMeasuredVariableById (FieldTrialServiceData *data_p, const char *mv_id_s)
{
	if (data_p -> dftsd_measured_variables_cache_p)
		{
			MeasuredVariableNode *node_p = (MeasuredVariableNode *) (data_p -> dftsd_measured_variables_cache_p -> ll_head_p);

			while (node_p)
				{
					if (strcmp (node_p -> mvn_id_s, mv_id_s) == 0)
						{
							return (node_p -> mvn_measured_variable_p);
						}
					else
						{
							node_p = (MeasuredVariableNode *) (node_p -> mvn_node.ln_next_p);
						}
				}
		}

	return NULL;
}


MeasuredVariable *GetCachedMeasuredVariableByName (FieldTrialServiceData *data_p, const char *name_s)
{
	MeasuredVariableNode *node_p = GetCachedMeasuredVariableNodeByName (data_p, name_s);

	if (node_p)
		{
			return (node_p -> mvn_measured_variable_p);
		}

	return NULL;
}


bool RemoveCachedMeasuredVariableByName (FieldTrialServiceData *data_p, const char *name_s)
{
	bool removed_flag = false;
	MeasuredVariableNode *node_p = GetCachedMeasuredVariableNodeByName (data_p, name_s);

	if (node_p)
		{
			LinkedListRemove (data_p -> dftsd_measured_variables_cache_p, & (node_p -> mvn_node));

			FreeMeasuredVariableNode (& (node_p -> mvn_node));
			removed_flag = true;
		}

	return removed_flag;
}



static MeasuredVariableNode *GetCachedMeasuredVariableNodeByName (const FieldTrialServiceData *data_p, const char *name_s)
{
	if (data_p -> dftsd_measured_variables_cache_p)
		{
			MeasuredVariableNode *node_p = (MeasuredVariableNode *) (data_p -> dftsd_measured_variables_cache_p -> ll_head_p);

			while (node_p)
				{
					const char *mv_name_s = GetMeasuredVariableName (node_p -> mvn_measured_variable_p);

					if (strcmp (mv_name_s, name_s) == 0)
						{
							return node_p;
						}
					else
						{
							node_p = (MeasuredVariableNode *) (node_p -> mvn_node.ln_next_p);
						}
				}
		}

	return NULL;
}



bool AddMeasuredVariableToCache (FieldTrialServiceData *data_p, MeasuredVariable *mv_p, MEM_FLAG mf)
{
	bool success_flag = false;

	if (data_p -> dftsd_measured_variables_cache_p)
		{
			MeasuredVariableNode *node_p = AllocateMeasuredVariableNode (mv_p, mf);

			if (node_p)
				{
					LinkedListAddTail (data_p -> dftsd_measured_variables_cache_p, & (node_p -> mvn_node));
					success_flag = true;
				}
			else
				{
					const char *name_s = GetMeasuredVariableName (mv_p);
					PrintErrors (STM_LEVEL_WARNING, __FILE__, __LINE__, "Failed to add \"%s\" to observations cache", name_s);
				}
		}
	else
		{
			success_flag = true;
		}

	return success_flag;
}


bool HasMeasuredVariableCache (FieldTrialServiceData *data_p)
{
	return (data_p -> dftsd_measured_variables_cache_p != NULL);
}




Treatment *GetCachedTreatmentByURL (FieldTrialServiceData *data_p, const char *url_s)
{
	if (data_p -> dftsd_treatments_cache_p)
		{
			TreatmentNode *node_p = (TreatmentNode *) (data_p -> dftsd_treatments_cache_p -> ll_head_p);

			while (node_p)
				{
					if (strcmp (node_p -> tn_treatment_url_s, url_s) == 0)
						{
							return (node_p -> tn_treatment_p);
						}
					else
						{
							node_p = (TreatmentNode *) (node_p -> tn_node.ln_next_p);
						}
				}
		}

	return NULL;
}


bool AddTreatmentToCache (FieldTrialServiceData *data_p, Treatment *treatment_p, MEM_FLAG mf)
{
	bool success_flag = false;

	if (data_p -> dftsd_treatments_cache_p)
		{
			TreatmentNode *node_p = AllocateTreatmentNode (treatment_p, mf);

			if (node_p)
				{
					LinkedListAddTail (data_p -> dftsd_treatments_cache_p, & (node_p -> tn_node));
					success_flag = true;
				}
			else
				{
					PrintErrors (STM_LEVEL_WARNING, __FILE__, __LINE__, "Failed to add \"%s\" to observations cache", treatment_p -> tr_ontology_term_p -> st_url_s);
				}
		}
	else
		{
			success_flag = true;
		}

	return success_flag;
}


bool HasTreatmentCache (FieldTrialServiceData *data_p)
{
	return (data_p -> dftsd_treatments_cache_p != NULL);
}



char *GetFullCacheFilename (const char *name_s, const FieldTrialServiceData *data_p)
{
	const char *cache_path_s = data_p -> dftsd_study_cache_path_s;
	const size_t cache_path_length = strlen (cache_path_s);
	char *filename_s = NULL;
	const char * const suffix_s = ".json";

	if (!DoesStringEndWith (name_s, suffix_s))
		{
			filename_s = ConcatenateStrings (name_s, suffix_s);

			if (!filename_s)
				{
					PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "ConcatenateStrings failed for \"%s\" and \"%s\"", name_s, suffix_s);
				}
		}
	else
		{
			filename_s = (char *) name_s;
		}

	if (filename_s)
		{
			/*
			 * Is it the full path?
			 */
			if (strncmp (cache_path_s, filename_s, cache_path_length) != 0)
				{
					char *full_filename_s = MakeFilename (cache_path_s, filename_s);

					if (!full_filename_s)
						{
							PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "MakeFilename failed for \"%s\" and \"%s\"", cache_path_s, filename_s);
						}

					if (filename_s != name_s)
						{
							FreeCopiedString (filename_s);
						}

					filename_s = full_filename_s;

				}
		}		/* if (filename_s) */

	return filename_s;
}


