= What is it? =
A simple volume control application exclusively for OpenBSD.

= Why and who is it for? =
I usually prefer a lightweight desktop environment setup. I wanted to have something that is more comfortable to use than sndioctl and is very lightweight.
It is not as feature rich as sndioctl, but has less dependencies than other volume control applications i.e. xfce4-mixer.

= Usage =
The UI is developed based on my experience and works well for my HW. The API for sndio exposes a bunch of controls and switches. I found some of these to be duplicates, so the UI doesn't show all.
The UI is self explanatory, but one invisible feature is that if you grab the slider with the right mouse button then "sibling" sliders are moving separately.

= Build =
Install fltk 1.3 (may work with 1.4 as well) and type make. That's it. There is no install target or manual page.
