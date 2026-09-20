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
 * row.h
 *
 *  Created on: 11 Sep 2018
 *      Author: billy
 */

#ifndef SERVICES_FIELD_TRIALS_INCLUDE_ROW_H_
#define SERVICES_FIELD_TRIALS_INCLUDE_ROW_H_


#include "material.h"
#include "plot.h"
#include "observation.h"
#include "treatment_factor_value.h"

#include "row.h"

struct Study;




typedef struct StandardRow
{
	Row sr_base;

	/**
	 * The row factor/category for this rack within
	 * its parent plot.
	 */
	uint32 sr_rack_index;

	/**
	 * The material that was planted in this row
	 */
	Material *sr_planted_material_p;

	/**
	 * The memory status for sr_planted_material_p
	 * @see sr_planted_material_p
	 */
	MEM_FLAG sr_planted_material_mem;


	/**
	 * The material that was planned for this row
	 */
	Material *sr_planned_material_p;

	/**
	 * The memory status for sr_planned_material_p
	 * @see sr_planned_material_p
	 */
	MEM_FLAG sr_planned_material_mem;

	LinkedList *sr_observations_p;

	LinkedList *sr_treatment_factor_values_p;

	uint32 sr_replicate_index;

	bool sr_replicate_control_flag;

	char *sr_store_code_s;

} StandardRow;


#ifndef DOXYGEN_SHOULD_SKIP_THIS

#ifdef ALLOCATE_STANDARD_ROW_TAGS
	#define STANDARD_ROW_PREFIX DFW_FIELD_TRIAL_SERVICE_API
	#define STANDARD_ROW_VAL(x)	= x
	#define STANDARD_ROW_CONCAT_VAL(x,y)	= x y
#else
	#define STANDARD_ROW_PREFIX extern
	#define STANDARD_ROW_VAL(x)
	#define STANDARD_ROW_CONCAT_VAL(x,y)
#endif

#endif 		/* #ifndef DOXYGEN_SHOULD_SKIP_THIS */



STANDARD_ROW_PREFIX const char *SR_RACK_INDEX_S STANDARD_ROW_VAL ("rack_index");


STANDARD_ROW_PREFIX const char *SR_REPLICATE_S STANDARD_ROW_VAL ("replicate");

STANDARD_ROW_PREFIX const char *SR_MATERIAL_ID_S STANDARD_ROW_VAL ("material_id");


STANDARD_ROW_PREFIX const char *SR_MATERIAL_S STANDARD_ROW_VAL ("material");


STANDARD_ROW_PREFIX const char *SR_PLANNED_MATERIAL_ID_S STANDARD_ROW_VAL ("planned_material_id");

STANDARD_ROW_PREFIX const char *SR_PLANNED_MATERIAL_S STANDARD_ROW_VAL ("planned_material");


STANDARD_ROW_PREFIX const char *SR_STORE_CODE_S STANDARD_ROW_VAL ("store_code");


STANDARD_ROW_PREFIX const char *SR_OBSERVATIONS_S  STANDARD_ROW_VAL ("observations");

STANDARD_ROW_PREFIX const char *SR_TREATMENTS_S  STANDARD_ROW_VAL ("treatments");


STANDARD_ROW_PREFIX const char *SR_REPLICATE_CONTROL_S STANDARD_ROW_VAL ("control");




STANDARD_ROW_PREFIX const char *SR_IMPORT_RACK_S STANDARD_ROW_VAL ("Rack");


STANDARD_ROW_PREFIX const char *SR_PLOT_INDEX_S STANDARD_ROW_VAL ("Plot index");


#ifdef __cplusplus
extern "C"
{
#endif


DFW_FIELD_TRIAL_SERVICE_LOCAL StandardRow *AllocateStandardRow (bson_oid_t *id_p, const uint32 rack_index, const uint32 study_index, const bool replicate_control_flag, const uint32 replicate, Material *material_p, MEM_FLAG material_mem, const char * const store_code_s, Plot *parent_plot_p);


DFW_FIELD_TRIAL_SERVICE_LOCAL StandardRow *GetStandardRowFromJSON (const json_t *json_p, Plot *plot_p, Material *material_p, const struct Study *study_p, const ViewFormat format, FieldTrialServiceData *data_p);

//DFW_FIELD_TRIAL_SERVICE_LOCAL bool SaveRow (Row *row_p, const FieldTrialServiceData *data_p, bool insert_flag);

DFW_FIELD_TRIAL_SERVICE_LOCAL bool AddObservationToStandardRow (StandardRow *row_p, Observation *observation_p);


DFW_FIELD_TRIAL_SERVICE_LOCAL void SetStandardRowGenotypeControl (StandardRow *row_p, bool control_flag);


DFW_FIELD_TRIAL_SERVICE_LOCAL bool IsStandardRowGenotypeControl (const StandardRow *row_p);


DFW_FIELD_TRIAL_SERVICE_LOCAL bool SetStandardRowStoreCode (StandardRow *row_p, const char * const store_code_s);


DFW_FIELD_TRIAL_SERVICE_LOCAL void UpdateStandardRow (StandardRow *row_p, const uint32 rack_index, const bool replicate_control_flag, const uint32 replicate, Material *material_p, MEM_FLAG material_mem, const char * const store_code_s);


DFW_FIELD_TRIAL_SERVICE_LOCAL bool AddTreatmentFactorValueToStandardRow (StandardRow *row_p, TreatmentFactorValue *tf_value_p);


DFW_FIELD_TRIAL_SERVICE_LOCAL bool AddStandardRowToJSON (const Row *row_p, json_t *row_json_p, const ViewFormat format, const FieldTrialServiceData *data_p);


DFW_FIELD_TRIAL_SERVICE_LOCAL bool AddStandardRowToPlotTable (const StandardRow *row_p, json_t *row_json_p, const FieldTrialServiceData *service_data_p);


DFW_FIELD_TRIAL_SERVICE_LOCAL bool SetStandardRowPlannedMaterial (StandardRow *row_p, Material *planned_material_p, MEM_FLAG planned_material_mem);


#ifdef __cplusplus
}
#endif


#endif /* SERVICES_FIELD_TRIALS_INCLUDE_ROW_H_ */
