/*
 * metadata.c
 *
 *  Created on: 3 Mar 2024
 *      Author: billy
 */


#include "metadata.h"

#include "dfw_util.h"

static const char * const S_METADATA_JSON_KEY_S = "metadata";


Metadata *AllocateMetadata (PermissionsGroup *permissions_group_p, User *user_p, const bool owns_user_flag, const char *timestamp_s)
{
	char *copied_timestamp_s = NULL;

	if ((timestamp_s == NULL) || (copied_timestamp_s = EasyCopyToNewString (timestamp_s)))
		{
			bool alloc_perms_flag = false;

			if (!permissions_group_p)
				{
					permissions_group_p = AllocatePermissionsGroup ();

					if (permissions_group_p)
						{
							alloc_perms_flag = true;
						}
				}

			if (permissions_group_p)
				{
					Metadata *metadata_p = (Metadata *) AllocMemory (sizeof (Metadata));

					if (metadata_p)
						{
							metadata_p -> me_timestamp_s = copied_timestamp_s;
							metadata_p -> me_user_p = user_p;
							metadata_p -> me_owns_user_flag = owns_user_flag;

							metadata_p -> me_permissions_p = permissions_group_p;

							return metadata_p;
						}

					if (alloc_perms_flag)
						{
							FreePermissionsGroup (permissions_group_p);
						}
				}


			if (copied_timestamp_s)
				{
					FreeCopiedString (copied_timestamp_s);
				}
		}		/* if ((timestamp_s == NULL) || (copied_timestamp_s = EasyCopyToNewString (timestamp_s))) */

	return NULL;
}


void FreeMetadata (Metadata *metadata_p)
{
	if (metadata_p -> me_permissions_p)
		{
			FreePermissionsGroup (metadata_p -> me_permissions_p);
		}

	if ((metadata_p -> me_user_p) && (metadata_p -> me_owns_user_flag))
		{
			FreeUser (metadata_p -> me_user_p);
		}

	if (metadata_p -> me_timestamp_s)
		{
			FreeCopiedString (metadata_p -> me_timestamp_s);
		}

	FreeMemory (metadata_p);
}



bool AddMetadataToJSON (const Metadata * const metadata_p, json_t *parent_json_p, const ViewFormat vf)
{
	bool success_flag = false;

	if (metadata_p)
		{
			json_t *metadata_json_p = GetMetadataAsJSON (metadata_p, vf);

			if (metadata_json_p)
				{
					if (json_object_set_new (parent_json_p, S_METADATA_JSON_KEY_S, metadata_json_p) == 0)
						{
							success_flag = true;
						}
					else
						{
							PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, metadata_json_p, "Failed to add metadata to json");
							json_decref (metadata_json_p);
						}
				}
			else
				{
					PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, parent_json_p, "GetMetadataAsJSON () failed");
				}
		}
	else
		{
			/* nothing to do */
			success_flag = true;
		}

	return success_flag;
}


Metadata *GetMetadataFromDefaultChildJSON (const json_t * const json_p, const ServiceData *data_p)
{
	Metadata *metadata_p = NULL;
	const json_t *metadata_json_p = json_object_get (json_p, S_METADATA_JSON_KEY_S);

	if (metadata_json_p)
		{
			metadata_p = GetMetadataFromJSON (metadata_json_p, data_p);

			if (!metadata_p)
				{
					PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, json_p, "GetMetadataFromJSON () failed");
				}

		}
	else
		{
			/* Check for the old format */
			const char *timestamp_s = GetJSONString (json_p, MONGO_TIMESTAMP_S);

			if (timestamp_s)
				{
					metadata_p = AllocateMetadata (NULL, NULL, false, timestamp_s);

					if (!metadata_p)
						{
							PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, json_p, "AllocateMetadata () with timestamp \"%s\" failed", timestamp_s);
						}

				}		/* if (timestamp_s) */

		}

	return metadata_p;
}


Metadata *GetMetadataFromJSON (const json_t * const json_p, const ServiceData *data_p)
{
	Metadata *metadata_p = NULL;
	const char *timestamp_s = GetJSONString (json_p, MONGO_TIMESTAMP_S);
	User *user_p = NULL;
	GrassrootsServer *grassroots_p = data_p -> sd_service_p -> se_grassroots_p;
	PermissionsGroup *perms_group_p = GetPermissionsGroupFromChildJSON (json_p, FT_PERMISSIONS_S, grassroots_p);
	bool successs_flag = false;

	json_t *user_json_p = json_object_get (json_p, FT_USER_S);

	if (user_json_p)
		{
			bson_oid_t *temp_id_p = GetNewUnitialisedBSONOid ();

			if (temp_id_p)
				{
					if (GetMongoIdFromJSON (user_json_p, temp_id_p))
						{
							user_p = GetUserById (grassroots_p, temp_id_p);

							if (user_p)
								{
									successs_flag = true;
								}

						}

					FreeBSONOid (temp_id_p);
				}

		}		/* if (user_json_p) */
	else
		{
			successs_flag = true;
		}

	if (successs_flag)
		{
			metadata_p = AllocateMetadata (perms_group_p, user_p, true, timestamp_s);

			if (!metadata_p)
				{
					PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "AllocateMetadata () failed");
				}
		}

	if (!metadata_p)
		{
			if (perms_group_p)
				{
					FreePermissionsGroup (perms_group_p);
				}
		}


	return metadata_p;
}



json_t *GetMetadataAsJSON (const Metadata * const metadata_p, const ViewFormat vf)
{
	json_t *metadata_json_p = json_object ();

	if (metadata_json_p)
		{
			if (SetNonTrivialString (metadata_json_p, MONGO_TIMESTAMP_S, metadata_p -> me_timestamp_s, true))
				{
					if ((metadata_p -> me_user_p == NULL) || (AddUserToJSON (metadata_p -> me_user_p, metadata_json_p, FT_USER_S, vf)))
						{
							if ((metadata_p -> me_permissions_p == NULL) || (AddPermissionsGroupToJSON (metadata_p -> me_permissions_p, metadata_json_p, metadata_p -> me_user_p, FT_PERMISSIONS_S, vf)))
								{
									return metadata_json_p;
								}
							else
								{
									PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, metadata_json_p, "AddPermissionsGroupToJSON () failed");
								}
						}
					else
						{
							PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, metadata_json_p, "AddUserToJSON () failed for \"%s\"", metadata_p -> me_user_p -> us_email_s);
						}

				}		/* if (SetNonTrivialString (metadata_json_p, MONGO_TIMESTAMP_S, metadata_p -> me_timestamp_s, true)) */

			json_decref (metadata_json_p);
		}

	return NULL;
}


void SetMetadataUser (Metadata *metadata_p, User *user_p, bool owns_user_flag)
{
	if ((metadata_p -> me_user_p) && (metadata_p -> me_owns_user_flag))
		{
			FreeUser (metadata_p -> me_user_p);
		}

	metadata_p -> me_user_p = user_p;
	metadata_p -> me_owns_user_flag = owns_user_flag;
}


bool MetadataHasUser (Metadata *metadata_p)
{
	return (metadata_p -> me_user_p != NULL);
}


