function hideBlockedRequests()
{
	let requests = [%1];
	let elements = Array.from(document.querySelectorAll('[src]'));

	for (let i = 0; i < elements.length; ++i)
	{
		for (let j = 0; j < requests.length; ++j)
		{
			if (elements[i].getAttribute('src').indexOf(requests[j]) > -1)
			{
				elements[i].style.cssText = 'display:none !important';
			}
		}
	}
}

hideBlockedRequests();
