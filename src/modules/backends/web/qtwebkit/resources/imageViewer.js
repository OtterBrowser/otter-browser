(function (window)
{
	var image = document.querySelector('img');
	var drag = null;
	var ignore = false;

	if (image && !image.classList.contains('imageViewer'))
	{
		image.classList.add('imageViewer');
		image.addEventListener('click', function(event)
		{
			if (drag)
			{
				return;
			}

			image.removeAttribute('width');
			image.removeAttribute('height');
			image.removeAttribute('style');

			var documentSize = [window.innerWidth, window.innerHeight];
			var imageSize = [image.naturalWidth, image.naturalHeight];

			if (imageSize[0] <= documentSize[0] && imageSize[1] <= documentSize[1])
			{
				image.classList.remove('zoomedIn');
				image.classList.remove('zoomedOut');
			}
			else if (!ignore)
			{
				if (image.classList.contains('zoomedOut'))
				{
					image.classList.add('zoomedIn');
					image.classList.remove('zoomedOut');
				}
				else
				{
					image.classList.add('zoomedOut');
					image.classList.remove('zoomedIn');
				}
			}

			ignore = false;
		});
		image.click();

		this.addEventListener('resize', function()
		{
			ignore = true;
			image.click();
		});

		this.addEventListener('mousedown', function(event)
		{
			if (image.classList.contains('zoomedIn') && event.button === 0)
			{
				drag = {
					oldX: (event.screenX + this.scrollX),
					oldY: (event.screenY + this.scrollY)
				};
			}
		});

		this.addEventListener('dragstart', function(event)
		{
			if (image.classList.contains('zoomedIn'))
			{
				event.preventDefault();
			}
		});

		this.addEventListener('mousemove', function(event)
		{
			if (drag === null)
			{
				return;
			}

			image.classList.add('drag');

			this.scrollTo((drag.oldX - event.screenX), (drag.oldY - event.screenY));
		});

		this.addEventListener('mouseup', function(event)
		{
			if (drag)
			{
				event.preventDefault();

				image.classList.remove('drag');

				drag = null;
			}
		});
	}
})(window);
