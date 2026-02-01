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
 * material.h
 *
 *  Created on: 11 Sep 2018
 *      Author: billy
 */

#ifndef SERVICES_FIELD_TRIALS_INCLUDE_MATERIAL_H_
#define SERVICES_FIELD_TRIALS_INCLUDE_MATERIAL_H_





#include "dfw_field_trial_service_data.h"
#include "dfw_field_trial_service_library.h"
#include "jansson.h"
#include "typedefs.h"
#include "study.h"
#include "gene_bank.h"

/*
 *
 * Species name
 * Germplasm ID
 * Type of Material
 * Reason for selection
 * Generation
 * Seed Supplier
 * Source of Seed
 * Germplasm Origin
 * In GRU?
 * GRU Accession
 * TGW
 * Seed Treatment
 * Cleaned
			keyword list	F2, F3 etc	Organisation	Trial or Organisation	Organisation	Y/N	ID	Number	Treatment 	Y/N
Triticum aestivum	BT1604	Synthetic Derived	yield, drought tolerance		Rothamsted	17/18 Trial	NIAB
Triticum aestivum	Skyfall	Elite	control		RAGT	Commercial	RAGT			150	Azole fungicide X

 *
 */


typedef struct MaterialStudyDetails
{
	char *ma_germplasm_id_s;

	const Study *ma_parent_area_p;

	char *ma_species_name_s;

	char *ma_type_s;

	char *ma_selection_reason_s;

	char *ma_generation_s;

	char *ma_seed_supplier_s;

	char *ma_seed_source_s;

	char *ma_germplasm_origin_s;

	bool ma_in_gru_flag;

	uint32 ma_tgw;

	char *ma_seed_treatment_s;

	bool ma_cleaned_flag;

	/**
	 * Optional full name of a toolkit line.
	 */
	char *ma_primary_name_s;


	/**
   * Optional simpler code for a toolkit line
   */
  char *ma_secondary_name_s;

	/**
	 * Optional Breeders Toolkit Number if the line is selected for the toolkit
 	 */
	char *ma_tertiary_name_s;

	/**
	 * Store code from the GRU
	 */
	char *ma_store_code_s;


}  MaterialStudyDetails;


typedef struct MaterialStudyDetailsNode
{
	ListItem msdn_node;

	MaterialStudyDetails *msdn_details_p;
} MaterialStudyDetailsNode;


typedef struct Material
{
	bson_oid_t *ma_id_p;

	bson_oid_t *ma_gene_bank_id_p;

	char *ma_accession_s;

} Material;





#ifndef DOXYGEN_SHOULD_SKIP_THIS

#ifdef ALLOCATE_MATERIAL_TAGS
	#define MATERIAL_PREFIX DFW_FIELD_TRIAL_SERVICE_LOCAL
	#define MATERIAL_VAL(x)	= x
	#define MATERIAL_CONCAT_VAL(x,y)	= x y
#else
	#define MATERIAL_PREFIX extern
	#define MATERIAL_VAL(x)
	#define MATERIAL_CONCAT_VAL(x,y)
#endif

#endif 		/* #ifndef DOXYGEN_SHOULD_SKIP_THIS */



MATERIAL_PREFIX const char *MA_ID_S MATERIAL_VAL ("id");

MATERIAL_PREFIX const char *MA_ACCESSION_S MATERIAL_VAL ("accession");

MATERIAL_PREFIX const char *MA_BARCODE_S MATERIAL_VAL ("barcode");

MATERIAL_PREFIX const char *MA_PEDIGREE_S MATERIAL_VAL ("pedigree");

MATERIAL_PREFIX const char *MA_GENE_BANK_ID_S MATERIAL_VAL ("gene_bank_id");

MATERIAL_PREFIX const char *MA_GENE_BANK_S MATERIAL_VAL ("gene_bank");

MATERIAL_PREFIX const char *MA_EXPERIMENTAL_AREA_ID_S MATERIAL_VAL ("area_id");

MATERIAL_PREFIX const char *MA_GERMPLASM_ID_S MATERIAL_VAL ("germplasm_id");




MATERIAL_PREFIX const char *MA_SPECIES_S MATERIAL_VAL ("http://purl.obolibrary.org/obo/NCIT_C45293");

MATERIAL_PREFIX const char *MA_TYPE_S MATERIAL_VAL ("type");
MATERIAL_PREFIX const char *MA_SELECTION_REASON_S MATERIAL_VAL ("selection_reason");
MATERIAL_PREFIX const char *MA_GENERATION_S MATERIAL_VAL ("generation");
MATERIAL_PREFIX const char *MA_SEED_SUPPLIER_S MATERIAL_VAL ("seed_supplier");
MATERIAL_PREFIX const char *MA_SEED_SOURCE_S MATERIAL_VAL ("seed_source");
MATERIAL_PREFIX const char *MA_GERMPLASM_ORIGIN_S MATERIAL_VAL ("germplasm_origin");
MATERIAL_PREFIX const char *MA_IN_GRU_S MATERIAL_VAL ("in_gru");
MATERIAL_PREFIX const char *MA_TGW_S MATERIAL_VAL ("tgw");
MATERIAL_PREFIX const char *MA_SEED_TREATMENT_S MATERIAL_VAL ("seed_treatment");

MATERIAL_PREFIX const char *MA_PRIMARY_NAME_S MATERIAL_VAL("primary_name");
MATERIAL_PREFIX const char *MA_SECONDARY_NAME_S MATERIAL_VAL("secondary_name");
MATERIAL_PREFIX const char *MA_TERTIARY_NAME_S MATERIAL_VAL("tertiary_name");
MATERIAL_PREFIX const char *MA_STODE_CODE_S MATERIAL_VAL("store_code");

MATERIAL_PREFIX const char *MA_CLEANED_NAME_S MATERIAL_VAL ("cleaned");




#ifdef __cplusplus
extern "C"
{
#endif


DFW_FIELD_TRIAL_SERVICE_LOCAL	Material *AllocateMaterial (bson_oid_t *id_p, const char *accession_s, const char *species_s, const char *type_s, const char *selection_reason_s, const char *generation_s, const char *supplier_s, const char *source_s, const char *germplasm_origin_s, const char *treatment_s, bool gru_flag, bool cleaned_flag, uint32 tgw, const Study *area_p, const bson_oid_t *gene_bank_id_p, const FieldTrialServiceData *data_p);


DFW_FIELD_TRIAL_SERVICE_LOCAL Material *AllocateMaterialByGermplasmID (bson_oid_t *id_p, const char *germplasm_id_s, const Study *area_p, const FieldTrialServiceData *data_p);


DFW_FIELD_TRIAL_SERVICE_LOCAL Material *AllocateMaterialByAccession (bson_oid_t *id_p, const char *accession_s, bson_oid_t *gene_bank_id_p, const FieldTrialServiceData *data_p);



DFW_FIELD_TRIAL_SERVICE_LOCAL bool SetMaterialAccession (Material *material_p, const char * const accession_s);

/*
DFW_FIELD_TRIAL_SERVICE_LOCAL bool SetMaterialPedigree (Material *material_p, const char * const pedigree_s);

DFW_FIELD_TRIAL_SERVICE_LOCAL bool SetMaterialBarcode (Material *material_p, const char * const barcode_s);

DFW_FIELD_TRIAL_SERVICE_LOCAL bool SetMaterialInternalName (Material *material_p, const char * const internal_name_s);
*/

DFW_FIELD_TRIAL_SERVICE_LOCAL void FreeMaterial (Material *material_p);

DFW_FIELD_TRIAL_SERVICE_LOCAL json_t *GetMaterialAsJSON (const Material *material_p, const ViewFormat format, const FieldTrialServiceData *data_p);

DFW_FIELD_TRIAL_SERVICE_LOCAL Material *GetMaterialFromJSON (const json_t *json_p, const ViewFormat format, const FieldTrialServiceData *data_p);


DFW_FIELD_TRIAL_SERVICE_LOCAL bool SaveMaterial (Material *material_p, const FieldTrialServiceData *data_p);

DFW_FIELD_TRIAL_SERVICE_LOCAL Material *LoadMaterial (const int32 material_id, FieldTrialServiceData *data_p);


DFW_FIELD_TRIAL_SERVICE_LOCAL Material*GetOrCreateMaterialByInternalName (const char *material_s, Study *area_p, const FieldTrialServiceData *data_p);

DFW_FIELD_TRIAL_SERVICE_LOCAL Material *GetOrCreateMaterialByAccession (const char *accession_s, GeneBank *gene_bank_p, const FieldTrialServiceData *data_p);


DFW_FIELD_TRIAL_SERVICE_LOCAL Material *GetMaterialByGermplasmID (const char *material_s, Study *area_p, const FieldTrialServiceData *data_p);

DFW_FIELD_TRIAL_SERVICE_LOCAL Material *GetMaterialById (const bson_oid_t *material_id_p, const FieldTrialServiceData *data_p);


DFW_FIELD_TRIAL_SERVICE_LOCAL Material *GetMaterialByAccession (const char *accession_s, GeneBank *gene_bank_p, const bool case_sensitive_flag, const FieldTrialServiceData *data_p);


DFW_FIELD_TRIAL_SERVICE_LOCAL bool IsMaterialComplete (const Material * const material_p);


DFW_FIELD_TRIAL_SERVICE_LOCAL json_t *GetAlStudyMaaterialIdss (const Study * const study_p, const FieldTrialServiceData *data_p);


#ifdef __cplusplus
}
#endif


#endif /* SERVICES_FIELD_TRIALS_INCLUDE_MATERIAL_H_ */
