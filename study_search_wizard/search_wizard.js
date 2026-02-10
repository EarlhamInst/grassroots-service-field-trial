
function AccessionChanged (accession_element)
{
	console.log (accession_element.value);
}

/**
 * Check the number is odd
 *
 * @param {Int} n - Number input.
 */
function isOdd(n) 
{
	return Math.abs(n % 2) == 1;
}

const S_DEBOUNCE_DELAY = 200;

const SearchGrassroots = Debounce (RealKeywordSearchGrassroots, S_DEBOUNCE_DELAY);



/**
 * Ajax search services for searching Treatments and Measured Variables
 *
 * @param {String} type - Define Treatments or Measured Variables.
 */
async function RealKeywordSearchGrassroots (query, facet_type) 
{
	console.log ("RealKeywordSearchGrassroots: " + query);

	let input_tail = "";

	if (isOdd((query.match(/\"/g) || []).length)) 
		{
			input_tail = "\"";
		}
	
	if (((query.match(/\"/g) || []).length) == 0) 
		{
			input_tail = "*";
		}

	console.log ("query: " + query + " tail: " + input_tail);

	if (query.length > 1) 
		{

			let submit_json = 
				{
					"services": [{
						"start_service": true,
						"so:name": "Search Grassroots",
						"parameter_set": {
						"level": "simple",
						"parameters": 
							[{
								"param": "SS Keyword Search",
								"current_value": query + input_tail
							}, {
								"param": "SS Facet",
								"current_value": facet_type
							}]
						}
					}]
				};

			let req_body = JSON.stringify (submit_json);

		//	console.log ("req_body: " + req_body);

		
			document.getElementById("loader").style.visibility = "visible";
			//document.getElementById("loader").innerHTML = "Loading...";

			const response = await fetch ("http://localhost:2000/grassroots/private_backend", {
				method: "POST",
				body: req_body,
			});

	//		console.log ("req_body: " + req_body);

			if (response.ok) 
				{
					let res_json = await response.json ();
	//				console.log ("res_json: " + JSON.stringify (res_json));
					LoadSearchResults (res_json);
				}
		 	else 
				{
					console.log ("response status: " + response.status);
				}

			document.getElementById("loader").style.visibility = "hidden";
			//document.getElementById("loader").innerHTML = "";

		} 
	else 
		{
			document.getElementById ("phenotypes_tbody").innerHTML = "";
		}
}


function LoadSearchResults (response_json) 
{
	const results = response_json.results;

	if (results) {
		let table = "";

		if (Array.isArray (results)) {
			if (results.length == 1) {
				const hits = results[0].results;
				
				let row_class=""
			
				for (let i in hits) {
					const hit = hits [i];
					const data = hit.data;

//					console.log ("hit: " + JSON.stringify (hit));
//					console.log ("data: " + JSON.stringify (data));
					
				
					let tr = "<tr onclick=\"SelectRow (this)\"";

					if (i % 2 == 1) 
						{
							tr += " class=\"odd\"";
						}
				
					tr += " data-var-name=\"data.variable [\"so:name\"]\">" + 
						"<td>" + data ["so:name"] + "</td>\n" +
						"<td>" + data.variable ["so:name"] + "</td>\n" +
						"<td>" + data.trait ["so:name"] + "</td>\n" +
						"<td>" + data.trait ["so:description"] + "</td>\n";
						"<tr>\n";

					table = table + tr;

				}

				

//				console.log ("table: " + table)

				document.getElementById ("phenotypes_tbody").innerHTML = table;

			}

		}
	}

}


function SelectRow (table_row) 
{
//	console.log ("selected table_row: ");
//	console.log (table_row.innerHTML);

	let selected_variable = table_row.cells [1].innerHTML;
//	console.log ("variable: " + selected_variable);

	let trait_name = table_row.cells [2].innerHTML;
//	console.log ("trait_name: " + trait_name);

	let trait_description = table_row.cells [3].innerHTML;
//	console.log ("trait_description: " + trait_description);

	let phenotypes_list = document.getElementById ("selected_phenotypes");

	console.log ("var name: " + selected_variable);

	const var_li = document.querySelector(`#selected_phenotypes li[data-var-name="${selected_variable}"]`);

	if (var_li !== null) 
		{	
			console.log (selected_variable + " is already on list");
		}
	else
		{
			let phenotype_entry = document.createElement ("li");

			phenotype_entry.setAttribute ("title", trait_description);
			phenotype_entry.setAttribute ("data-var-name", selected_variable);
			
			/*
				Create the delete button
			*/
			let remove_button = document.createElement ("input");
			remove_button.type = "image"
			remove_button.setAttribute ("src", "/grassroots/images/aiss/delete");
			
			remove_button.setAttribute ("onclick", "RemoveSelectedPhenotype (this.parentElement)");
			remove_button.setAttribute ("title", "Remove " + selected_variable + " from selected phenotypes");


		//	remove_button.innerHTML = "delete";
			phenotype_entry.appendChild (remove_button);
			phenotype_entry.appendChild (document.createTextNode (selected_variable));
			phenotypes_list.appendChild (phenotype_entry);
		}



	if (table_row.classList.contains ("selected"))
		{
			table_row.classList.remove ("selected");
		}
	else
		{
			table_row.classList.add ("selected");
		}
}


function RemoveSelectedPhenotype (list_entry)
{
	let phenotypes_list = list_entry.parentElement;
	
	console.log ("list_entry " + list_entry);
	console.log ("phenotypes_list " + phenotypes_list);

	phenotypes_list.removeChild (list_entry);

	const table_row = document.querySelector(`#phenotypes_tbody tr[data-var-name="${selected_variable}"]`);

	if (table_row !== null)
		{
			table_row.classList.remove ("selected");
		}
}


/**
 * Instead of running a function every time, add a small delay before
 * running. If the user is typing in a search term this will then only 
 * call the server once they've paused typing rather than on every 
 * keystroke.
 * 
 * @param callback_fn The function to run after the given time of inactivity.
 * @param wait_time The time in milliseconds to use as the inactivity time.
 */
function Debounce (callback_fn, wait_time) 
{
	let timer;

	return function (...args) 
		{
			// cancel the timer
			clearTimeout (timer);

			// set a new timer
			timer = setTimeout (() => 
				{
					// run the callback function with its original args
					callback_fn.apply (this, args);
				}, 
				wait_time
			);
		};
}


function SearchGrassrootsHandler (event)
{
	// Cancel the default action

	var query = event.target.value;

	const facet = "Measured Variable";

	if (event.key === "Enter")
		{
			// Do the search immediately
			event.preventDefault ();
			RealKeywordSearchGrassroots (query, facet);
		}
	else
		{
			Debounce (RealSearchGrassroots, S_DEBOUNCE_DELAY, query, facet);
		}
}


function SearchStudies ()
{


}

