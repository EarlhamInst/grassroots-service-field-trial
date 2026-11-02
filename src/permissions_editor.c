/*
 * permissions_editor.c
 *
 *  Created on: 10 Jan 2024
 *      Author: billy
 */

#include "permissions_editor.h"
#include "string_array_parameter.h"
#include "string_parameter.h"
#include "streams.h"


static NamedParameterType PERMISSION_READ = { "PE Read", PT_STRING_ARRAY } ;
static NamedParameterType PERMISSION_WRITE = { "PE Write", PT_STRING_ARRAY } ;
static NamedParameterType PERMISSION_DELETE = { "PE Delete", PT_STRING_ARRAY } ;


static bool GetUserEmailsForUserList (const LinkedList * const users_p, char ***emails_sss);


static bool AddPermissionsParameter (const Permissions *permissions_p, const char * const name_s, const char * const display_name_s, const char * const description_s,
																		 LinkedList *all_users_p, ParameterGroup *perms_group_p, ParameterSet *param_set_p, const bool read_only_flag,
																		 FieldTrialServiceData *ft_data_p);

static OperationStatus UpdatePermissionsValues (Permissions *permissions_p, const char *param_s, ParameterSet *param_set_p, ServiceJob *job_p, GrassrootsServer *grassroots_p);

static bool UpdatePermissionsValuesAndStatus (Permissions *permissions_p, const char *param_s, ParameterSet *param_set_p, ServiceJob *job_p, GrassrootsServer *grassroots_p);



bool AddPermissionsEditor (PermissionsGroup *permissions_group_p, const char *id_s, ParameterSet *param_set_p, const bool read_only_flag, FieldTrialServiceData *ft_data_p)
{
	bool success_flag = true;
	const GrassrootsServer *grassroots_p = ft_data_p -> dftsd_base_data.sd_service_p -> se_grassroots_p;
	LinkedList *all_users_p = GetAllUsers (grassroots_p);
	ParameterGroup *perms_group_p = CreateAndAddParameterGroupToParameterSet ("Permissions",  false, & (ft_data_p -> dftsd_base_data), param_set_p);

	if (success_flag)
		{
			Permissions *permissions_p = permissions_group_p ? permissions_group_p -> pg_read_access_p : NULL;

			if (AddPermissionsParameter (permissions_p, PERMISSION_READ.npt_name_s, "Read", "Users with Read access",
																	 all_users_p, perms_group_p, param_set_p, read_only_flag, ft_data_p))
				{
					permissions_p = permissions_group_p ? permissions_group_p -> pg_write_access_p : NULL;

					if (AddPermissionsParameter (permissions_p, PERMISSION_WRITE.npt_name_s, "Write", "Users with Write access",
																			 all_users_p, perms_group_p, param_set_p, read_only_flag, ft_data_p))
						{
							permissions_p = permissions_group_p ? permissions_group_p -> pg_delete_access_p : NULL;

							if (AddPermissionsParameter (permissions_p, PERMISSION_DELETE.npt_name_s, "Delete", "Users with Delete Access",
																					 all_users_p, perms_group_p, param_set_p, read_only_flag, ft_data_p))
								{

								}
							else
								{
									PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "AddPermissionsParameter () failed for \"%s\"", PERMISSION_DELETE.npt_name_s);
								}

						}
					else
						{
							PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "AddPermissionsParameter () failed for \"%s\"", PERMISSION_WRITE.npt_name_s);
						}

				}
			else
				{
					PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "AddPermissionsParameter () failed for \"%s\"", PERMISSION_READ.npt_name_s);
				}
		}


	if (all_users_p)
		{
			FreeLinkedList (all_users_p);
		}

	return success_flag;
}


bool GetPermissionsEditorParameterTypeForNamedParameter (const char *param_name_s, ParameterType *pt_p)
{
	const NamedParameterType params [] =
		{
			PERMISSION_READ,
			PERMISSION_WRITE,
			PERMISSION_DELETE,
			NULL
		};

	return DefaultGetParameterTypeForNamedParameter (param_name_s, pt_p, params);
}


PermissionsGroup *GetPermissionsGroupFromPermissionsEditor (ParameterSet *param_set_p, ServiceJob *job_p, User *user_p, ServiceData *data_p)
{
	PermissionsGroup *perms_group_p = AllocatePermissionsGroup ();

	if (perms_group_p)
		{
			OperationStatus status = RunForPermissionEditor (param_set_p, perms_group_p, job_p, user_p, data_p);

			if ((status == OS_SUCCEEDED) || (status == OS_PARTIALLY_SUCCEEDED) || (status == OS_IDLE))
				{
					return perms_group_p;
				}

			FreePermissionsGroup (perms_group_p);
		}

	return NULL;
}


OperationStatus RunForPermissionEditor (ParameterSet *param_set_p, PermissionsGroup *permissions_group_p, ServiceJob *job_p, User *user_p, ServiceData *data_p)
{
	GrassrootsServer *grassroots_p = data_p -> sd_service_p -> se_grassroots_p;

	job_p -> sj_status = OS_IDLE;

	if (UpdatePermissionsValuesAndStatus (permissions_group_p -> pg_read_access_p, PERMISSION_READ.npt_name_s, param_set_p, job_p, grassroots_p))
		{
			if (UpdatePermissionsValuesAndStatus (permissions_group_p -> pg_write_access_p, PERMISSION_WRITE.npt_name_s, param_set_p, job_p, grassroots_p))
				{
					if (!UpdatePermissionsValuesAndStatus (permissions_group_p -> pg_delete_access_p, PERMISSION_DELETE.npt_name_s, param_set_p, job_p, grassroots_p))
						{
							PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "UpdatePermissionsValuesAndStatus () failed for \"%s\"", PERMISSION_DELETE.npt_name_s);
						}

				}
			else
				{
					PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "UpdatePermissionsValuesAndStatus () failed for \"%s\"", PERMISSION_WRITE.npt_name_s);
				}

		}
	else
		{
			PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "UpdatePermissionsValuesAndStatus () failed for \"%s\"", PERMISSION_READ.npt_name_s);
			job_p -> sj_status = OS_FAILED;
		}

	return job_p -> sj_status;
}


static OperationStatus UpdatePermissionsValues (Permissions *permissions_p, const char *param_s, ParameterSet *param_set_p, ServiceJob *job_p, GrassrootsServer *grassroots_p)
{
	OperationStatus status = OS_FAILED;
	const char **values_ss = NULL;
	size_t num_entries = 0;

	if (GetCurrentStringArrayParameterValuesFromParameterSet (param_set_p, param_s, &values_ss, &num_entries))
		{

			size_t i;
			const char **value_ss = values_ss;
			size_t num_successes = 0;


			ClearPermissions (permissions_p);

			for (i = 0; i < num_entries; ++ i, ++ value_ss)
				{
					const char *email_s = *value_ss;
					User *user_p = GetExistingUserByEmailAddress (grassroots_p, email_s);

					if (user_p)
						{
							if (AddUserToPermissions (permissions_p, user_p))
								{
									++ num_successes;
								}
							else
								{
									PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "AddPermissionsParameter () failed for \"%s\"", param_s);
									FreeUser (user_p);
								}
						}
					else
						{
							PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "AddPermissionsParameter () failed for \"%s\" with email \"%s\"", param_s, email_s);

							char *error_s = ConcatenateVarargsStrings ("No user found with email address: ", email_s, NULL);

							if (error_s)
								{
									AddParameterErrorMessageToServiceJob (job_p, param_s, PT_STRING_ARRAY, error_s);
									FreeCopiedString (error_s);
								}
						}

				}		/*for (i = 0; i < num_entries; ++ i, ++ value_ss) */

			if (num_successes == num_entries)
				{
					status = OS_SUCCEEDED;
				}
			else if (num_successes > 0)
				{
					status = OS_PARTIALLY_SUCCEEDED;
				}
			else
				{
					status = OS_FAILED;
				}
		}
	else
		{
			status = OS_IDLE;
		}

	return status;
}

/*
 * Static declarations
 */

static bool AddPermissionsParameter (const Permissions *permissions_p, const char * const name_s, const char * const display_name_s, const char * const description_s,
																		 LinkedList *all_users_p, ParameterGroup *perms_group_p, ParameterSet *param_set_p, const bool read_only_flag, FieldTrialServiceData *ft_data_p)
{
	bool success_flag = true;
	Parameter *param_p = NULL;
	char **emails_ss = NULL;
	uint32 num_emails = 0;

	if (permissions_p && (permissions_p -> pe_users_p) && ((num_emails = permissions_p -> pe_users_p -> ll_size) > 0))
		{
			if (!GetUserEmailsForUserList (permissions_p -> pe_users_p, &emails_ss))
				{
					PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "GetUserEmailsForUserList () failed for param \"%s\"", name_s);
					success_flag = false;
				}
		}

	if (success_flag)
		{
			param_p = EasyCreateAndAddStringArrayParameterToParameterSet (& (ft_data_p -> dftsd_base_data), param_set_p, perms_group_p,
																																		name_s, display_name_s, description_s, emails_ss, num_emails, PL_ALL);

			if (param_p)
				{
					param_p -> pa_read_only_flag = read_only_flag;

					if (all_users_p)
						{
							UserNode *user_node_p = (UserNode *) (all_users_p -> ll_head_p);

							while (user_node_p && success_flag)
								{
									const char * const email_s = user_node_p -> un_user_p -> us_email_s;

									if (CreateAndAddStringParameterOption (param_p, email_s, email_s))
										{
											user_node_p = (UserNode *) (user_node_p -> un_node.ln_next_p);
										}
									else
										{
											success_flag = false;
										}
								}
						}
				}
			else
				{
					PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to create parameter \"%s\"", name_s);
					success_flag = false;
				}

		}

	if (emails_ss)
		{
			FreeMemory (emails_ss);
		}

	return success_flag;
}


static bool GetUserEmailsForUserList (const LinkedList * const users_p, char ***emails_sss)
{
	bool success_flag = true;

	if (users_p)
		{
			const uint32 num_users = users_p -> ll_size;

			if (num_users > 0)
				{
					char **emails_ss = (char **) AllocMemoryArray (num_users, sizeof (char *));

					if (emails_ss)
						{
							UserNode *user_node_p = (UserNode *) (users_p -> ll_head_p);
							char **email_ss = emails_ss;

							while (user_node_p)
								{
									*email_ss = user_node_p -> un_user_p -> us_email_s;

									user_node_p = (UserNode *) (user_node_p -> un_node.ln_next_p);
									++ email_ss;
								}

							*emails_sss = emails_ss;
						}		/* if (emails_ss) */
					else
						{
							success_flag = false;
						}
				}		/* if (num_users > 0) */
		}

	return success_flag;
}



static bool UpdatePermissionsValuesAndStatus (Permissions *permissions_p, const char *param_s, ParameterSet *param_set_p, ServiceJob *job_p, GrassrootsServer *grassroots_p)
{
	bool success_flag = false;
	OperationStatus perms_status = UpdatePermissionsValues (permissions_p, PERMISSION_READ.npt_name_s, param_set_p, job_p, grassroots_p);

	if ((perms_status == OS_IDLE) || (perms_status == OS_PARTIALLY_SUCCEEDED) || (perms_status == OS_SUCCEEDED))
		{
			MergeOperationStatuses (& (job_p -> sj_status), perms_status);
			success_flag = true;
		}

	return success_flag;
}
