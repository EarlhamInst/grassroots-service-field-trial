NAME 		:= dfw_field_trial_service
DIR_BUILD :=  $(realpath $(dir $(lastword $(MAKEFILE_LIST)))/$(PLATFORM))
DIR_SRC := $(realpath $(DIR_BUILD)/../../../src)
DIR_INCLUDE := $(realpath $(DIR_BUILD)/../../../include)

ifeq ($(DIR_BUILD_CONFIG),)
export DIR_BUILD_CONFIG = $(realpath $(DIR_BUILD)/../../../../../build-config/unix/)
#export DIR_BUILD_CONFIG = /Users/tyrrells/Projects/grassroots/build-config/unix/
endif

include $(DIR_BUILD_CONFIG)/project.properties

# include the configuration details of any dependencies
-include $(DIR_BUILD)/user.prefs

# BEGIN LIBEXIF CONFIGURATION
ifneq ($(LIBEXIF_HOME),)
DIR_LIBEXIF := $(LIBEXIF_HOME)
else
DIR_LIBEXIF := /usr/local
endif
export DIR_LIBEXIF_INC := $(DIR_LIBEXIF)/include
export DIR_LIBEXIF_LIB := $(DIR_LIBEXIF)/lib
# END LIBEXIF CONFIGURATION


VPATH := $(DIR_SRC)

BUILD		:= debug

export DIR_INSTALL := $(DIR_GRASSROOTS_INSTALL)/services

VPATH	= \
	$(DIR_SRC) \
	

INCLUDES = \
	-I$(DIR_INCLUDE) \
	-I$(DIR_GRASSROOTS_USERS_INC) \
	-I$(DIR_GRASSROOTS_UUID_INC) \
	-I$(DIR_GRASSROOTS_MONGODB_INC) \
	-I$(DIR_GRASSROOTS_SQLITE_INC) \
	-I$(DIR_GRASSROOTS_UTIL_INC) \
	-I$(DIR_GRASSROOTS_UTIL_INC)/containers \
	-I$(DIR_GRASSROOTS_UTIL_INC)/io \
	-I$(DIR_GRASSROOTS_HANDLER_INC) \
	-I$(DIR_GRASSROOTS_SERVER_INC) \
	-I$(DIR_GRASSROOTS_SERVICES_INC) \
	-I$(DIR_GRASSROOTS_NETWORK_INC) \
	-I$(DIR_GRASSROOTS_SERVICES_INC)/parameters \
	-I$(DIR_GRASSROOTS_PLUGIN_INC) \
	-I$(DIR_GRASSROOTS_TASK_INC) \
	-I$(DIR_GRASSROOTS_GEOCODER_INC) \
	-I$(DIR_GRASSROOTS_FRICTIONLESS_INC) \
	-I$(DIR_JANSSON_INC) \
	-I$(DIR_UUID_INC) \
	-I$(DIR_MONGODB_INC) \
	-I$(DIR_BSON_INC) \
	-I$(DIR_GRASSROOTS_LUCENE_INC) \
	-I$(DIR_LIBEXIF_INC) \
	
SRCS 	= \
	blank_row.c \
	browse_programme_history.c \
	browse_trial_history.c \
	copy_study.c \
	crop.c \
	crop_jobs.c \
	crop_ontology.c \
	crop_ontology_tool.c \
	dfw_field_trial_service.c \
	dfw_field_trial_service_data.c \
	dfw_util.c \
	discard_row.c \
	edit_plot.c \
	field_trial.c \
	field_trial_jobs.c \
	field_trial_mongodb.c \
	gene_bank.c \
	gene_bank_jobs.c \
	guard_row.c \
	handbook_generator.c \
	image_util.c \
	indexing.c \
	instrument.c \
	integer_observation.c \
	json_processor.c \
	location.c \
	location_jobs.c \
	material.c \
	material_jobs.c \
	measured_variable.c \
	measured_variable_jobs.c \
	metadata.c \
	numeric_observation.c \
	observation.c \
	observation_metadata.c \
	permissions_editor.c \
	person.c \
	person_jobs.c \
	phenotype_jobs.c \
	phenotype_statistics.c \
	plot.c \
	plot_jobs.c \
  plots_cache.c \
	programme.c \
	programme_jobs.c \
	row.c \
	row_jobs.c \
	row_processor.c \
	search_service.c \
	standard_row.c \
	string_observation.c \
	study.c \
	study_jobs.c \
	study_manager.c \
	submit_crop.c \
	submit_field_trial.c \
	submit_gene_bank.c \
	submit_location.c \
	submit_material.c \
	submit_plots.c \
	submit_program.c \
	submit_study.c \
	submit_measured_variables.c \
	submit_treatment.c \
	submit_treatment_factor.c \
	time_observation.c \
	treatment.c \
	treatment_factor.c \
	treatment_jobs.c \
	treatment_factor_jobs.c \
	treatment_factor_value.c \
		
CPPFLAGS += -DDFW_FIELD_TRIAL_LIBRARY_EXPORTS




LIB_LDFLAGS = 	\
	-L$(DIR_GRASSROOTS_UTIL_LIB) -l$(GRASSROOTS_UTIL_LIB_NAME) \
	-L$(DIR_GRASSROOTS_UUID_LIB) -l$(GRASSROOTS_UUID_LIB_NAME) \
	-L$(DIR_GRASSROOTS_USERS_LIB) -l$(GRASSROOTS_USERS_LIB_NAME) \
	-L$(DIR_GRASSROOTS_SERVICES_LIB) -l$(GRASSROOTS_SERVICES_LIB_NAME) \
	-L$(DIR_GRASSROOTS_SERVER_LIB) -l$(GRASSROOTS_SERVER_LIB_NAME) \
	-L$(DIR_GRASSROOTS_MONGODB_LIB) -l$(GRASSROOTS_MONGODB_LIB_NAME)  \
	-L$(DIR_GRASSROOTS_GEOCODER_LIB) -l$(GRASSROOTS_GEOCODER_LIB_NAME) \
	-L$(DIR_GRASSROOTS_LUCENE_LIB) -l$(GRASSROOTS_LUCENE_LIB_NAME) \
	-L$(DIR_GRASSROOTS_HANDLER_LIB) -l$(GRASSROOTS_HANDLER_LIB_NAME) \
	-L$(DIR_GRASSROOTS_FRICTIONLESS_LIB) -l$(GRASSROOTS_FRICTIONLESS_LIB_NAME) \
	-L$(DIR_JANSSON_LIB) -ljansson \
	-L$(DIR_MONGODB_LIB) -l$(MONGO_LIB_NAME) \
	-L$(DIR_BSON_LIB) -l$(BSON_LIB_NAME) \
	-L$(DIR_LIBEXIF_LIB) -lexif \
	-L$(DIR_UUID_LIB) -luuid \
	-L$(DIR_GRASSROOTS_NETWORK_LIB) -l$(GRASSROOTS_NETWORK_LIB_NAME) \
	-lcurl
	
LDFLAGS += $(LIB_LDFLAGS)

APP_LDFLAGS = $(LIB_LDFLAGS) \
	-L$(DIR_PCRE2_LIB) -l$(PCRE2_LIB_NAME) \
	-lcurl	
	
ifeq ($(IRODS_ENABLED), 1)
APP_LDFLAGS += -L$(DIR_GRASSROOTS_IRODS_LIB) -l$(GRASSROOTS_IRODS_LIB_NAME)
endif
	

ifeq ($(SLURM_DRMAA_ENABLED),1)
APP_LDFLAGS += \
	-L$(DIR_GRASSROOTS_DRMAA_LIB) -l$(GRASSROOTS_DRMAA_LIB_NAME) \
	-L$(DIR_SLURM_DRMAA_LIB) -ldrmaa
else ifeq ($(LSF_DRMAA_ENABLED),1)
APP_LDFLAGS += \
	-L$(DIR_GRASSROOTS_DRMAA_LIB) -l$(GRASSROOTS_DRMAA_LIB_NAME) \
	-L$(DIR_LSF_DRMAA_LIB) -ldrmaa
else ifeq ($(HTCONDOR_DRMAA_ENABLED),1)
APP_LDFLAGS += \
	-L$(DIR_GRASSROOTS_DRMAA_LIB) -l$(GRASSROOTS_DRMAA_LIB_NAME) \
	-L$(DIR_HTCONDOR_DRMAA_LIB) -ldrmaa 
endif	
	
SCALE_CLASS_APP_LDFLAGS = \
	-L$(DIR_GRASSROOTS_UTIL_LIB) -l$(GRASSROOTS_UTIL_LIB_NAME) \
	-L$(DIR_GRASSROOTS_PARAMS_LIB) -l$(GRASSROOTS_PARAMS_LIB_NAME) \
	-L$(DIR_GRASSROOTS_MONGODB_LIB) -l$(GRASSROOTS_MONGODB_LIB_NAME)  \
	-L$(DIR_JANSSON_LIB) -ljansson \
	-L$(DIR_MONGODB_LIB) -l$(MONGO_LIB_NAME) \
	-L$(DIR_BSON_LIB) -l$(BSON_LIB_NAME) \
	-L$(DIR_PCRE2_LIB) -l$(PCRE2_LIB_NAME) 
	
	
	
PLOT_ROW_APP_LDFLAGS += $(APP_LDFLAGS) \
	-lcurl


ifneq ($(DIR_MARTI),)
	SRCS += marti_util.c
	INCLUDES += -I$(DIR_MARTI)/include
	CPPFLAGS += -DENABLE_MARTI=1
	LDFLAGS += -L$(DIR_MARTI)/build/unix/$(PLATFORM)/$(BUILD) -lmarti_service
endif



all:: 
	make -C $(DIR_GRASSROOTS_GEOCODER_BUILD) all

sanitiser: all
	gcc $(DIR_SRC)/treatment_sanitiser.c -o treatment_sanitiser -DUNIX=1 $(INCLUDES) $(APP_LDFLAGS) -L$(DIR_BUILD)/$(BUILD) -l$(NAME)


importer: all
	$(CC) $(DIR_SRC)/importer.c -o $(DIR_BUILD)/$(BUILD)/importer -DUNIX=1 -Wall -Wshadow -Wextra  -g -O0 -ggdb  $(CPPFLAGS)  $(INCLUDES) -L$(DIR_BUILD)/$(BUILD) -l$(NAME)  $(APP_LDFLAGS)
	
plot_row_merger: all
	$(CC) $(DIR_SRC)/merge_plot_row_collections.c -o $(DIR_BUILD)/$(BUILD)/merge_plot_row_collections -DUNIX=1 -Wall -Wshadow -Wextra  -g -O0 -ggdb  $(CPPFLAGS)  $(INCLUDES) -L$(DIR_BUILD)/$(BUILD) -l$(NAME) $(PLOT_ROW_APP_LDFLAGS)
	

scale_class_app: 
	gcc $(DIR_SRC)/mongo_scale_class_processor.c -o $(DIR_BUILD)/$(BUILD)/mongo_scale_class_processor  -g -O0 -ggdb -DUNIX=1 -Wall -Wshadow -Wextra $(CPPFLAGS) $(INCLUDES) $(SCALE_CLASS_APP_LDFLAGS) 


include $(DIR_BUILD_CONFIG)/generic_makefiles/shared_library.makefile



