var form = document.createElement('form');
form.setAttribute('method', 'post');
form.setAttribute('action', '%1');

var parameters = '%2'.split('&');

for (var i = 0; i < parameters.length; ++i)
{
	var parameter = parameters[i].split('=');
	var input = document.createElement('input');
	input.setAttribute('type', 'hidden');
	input.setAttribute('name', parameter[0]);
	input.setAttribute('value', decodeURIComponent(parameter[1]));

	form.appendChild(input);
}

form.submit();
