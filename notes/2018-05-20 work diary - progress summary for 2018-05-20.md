# Progress summary 2018-05-20

[Algorithmic fading trails using FastLED libraries][vid1]

[vid1]: https://vimeo.com/271001287

## DimTrail() function

Last year, we were seriously concerned about the performance of the Arduino while animating 4-20 sprites at a time, and so we pre-rendered as much as possible as a precaution. As it turns out, we're nowhere near taxing the performance of the processor: our bottleneck for performance is `FastLED::Show`. 

So this year, instead of a pre-rendered pixel trail for the faerie sprites, I added a `DimTrail()` method. 

When I move the `pattern` CRGB sprite one pixel fowards, it doesn't turn off the LED that, in the previous interval, was the rear-most pixel of the `pattern`. So I call `DimTrail()` on that pixel, and it gets faded by ~50% using a FastLED function. The `DimTrail()` method is recursive, so it then steps back one pixel and fades again, and so on, until it detects that it's reached a black pixel, at which point it exits.  

```
	// add int direction and make tailpixel += direction, and when direction is "backwards", offset the dimtrail by 3 pixels to prevent dimming the sprite
	void DimTrail(int tailPixel, int dimFactor, int direction) {
		if (tailPixel < 0) return;
		if (tailPixel > NUM_LEDS) return;
		if (! leds[tailPixel]) return;
		
		leds[tailPixel].fadeToBlackBy(dimFactor);
		tailPixel += direction;
		DimTrail(tailPixel, dimFactor, direction);
	}
```
The FastLED `fadeToBlackBy` function operates in 1/256ths, meaning that if you feed it 128, it fades 50%, if you feed it 64, it fades 25%, and so on. So I also wrote a function that maps `updateInterval` (the variable framerate) to `dimFactor`, so that now when faerie sprites move slowly, they start with a short trail, and when they accelerate to move quickly, they stretch out a long trail behind them. 

That's great, but when the sprites *stop* for their idle animation, the trail fade doesn't look right. 

So I created a `FadeOutTrail()` method, which itself calls `DimTrail()` recursively. 

	void FadeOutTrail(int tailPixel, int dimFactor, int direction) {
		if (tailPixel < 0) return;
		if (tailPixel > NUM_LEDS) return;
		if (! leds[tailPixel]) return;
		
		// Recursively run DimTrail() at tailPixel+1, so trail fades from the dim end
		DimTrail(tailPixel, dimFactor, direction);
		FadeOutTrail(tailPixel + direction, dimFactor, direction);
	}

The result of this method is that, instead of staying the same length and fading out awkwardly, when the sprites stop and idle, the trails shrink from the far end inwards. It's a subtle improvement, but a nice one. 

## Algorithmic braking

Now that I've done some pixel animation of how I want the faerie sprites to move, I've worked out that I like to have them slow down and stop over a range of about 4-8 pixels, with the trails fading as they slow. I'm now working on slowing the sprites down by manipulating the framerate (which is how they accelerate). 

I just put in some movement logic where the sprites calculate their distance from their destination pixel, and when that is within a certain percentage of the overall travel distance, it starts braking. 

However, currently deceleration is handled by a single factor; how many milliseconds do you add or subtract to the `updateInterval` on each call of `updateTravel()`? 

For acceleration from stop, it's simple enough to start at about 40ms, and subtract 1ms per interval as you travel. The result is a reasonably natural enough acceleration towards "cruising speed". 

For making the sprite slow down to a stop, however, that doesn't really result in a pleasing deceleration curve. 

[Algorithmic brake at 15% of total distance travelled][vid2]

[vid2]: https://vimeo.com/271006221

After some work, I came up with this acceleration method: 

    int AccelerateTravel() {
		// Decide whether braking or accelerating
		if (currentDistance < totalTravelDistance * brakePercentage) {
			if (!isBraking) {
				isBraking = true;
				brakeDistance = abs(currentDistance - totalTravelDistance);
				brakePixel = currentPixel;
			}
		} else {
			isBraking = false;
		}
		// Accelerate or brake by returning positive or negative values to subtract from updateInterval
    	if (! isBraking) {
			int x = abs(currentDistance - totalTravelDistance);
    		return round(sqrt(x) * accelerationFactor);
    	} else if (isBraking) {
			int x = abs(currentPixel - brakePixel);
    		return -1 * round(sqrt(x) * brakeFactor);
    	}
    }

Whether braking or accelerating, the change to `updateInterval` is a function of the distance traveled from the point where you started accelerating. `accelerationFactor` and `brakeFactor` are semi-randomized values assigned on sprite creation, whose purpose is to make sure that, even if two sprites start at the same pixel headed in the same direction, they'll have slightly different acceleration and brake speeds, so they'll move a bit differently from each other. I'm still dialing in what those values should be. 

## Algorithmic idle animation

After the faerie sprite in the above video slows to a stop, it idles across 3 pixels, back and forth. I sketched this in terms of pixel values from the `colorPalette` array in my new notebook, and my friend [Scott Longely][sl] translated it into a set of simple algorithms for each pixel. 

[sl]: http://s-a-w-s.blogspot.com

That isn't the full desired result, though. What I need to do next is to write a `colorFade()` function to transition smoothly between pixel values. 

I've found example code as gists on GitHub. Both use FastLED functions for the fade, which have so far resulted in good animation performance, so I'm game to see how they go. 

* [Example One](https://gist.github.com/kriegsman/1f7ccbbfa492a73c015e)
* [Example Two](https://gist.github.com/kriegsman/d0a5ed3c8f38c64adcb4837dafb6e690)

I have so far not looked at the code at all to see how suitible it may be. So that'll happen soon. 

## Transition from idle to travel

There's a visual discontinuity at the moment where the animation transitions from `isIdling` to `false`; at that moment, it chooses a new destination, and draws the `travel` sprite. Unfortunately, the last frame of the idle animation and the travel sprite do not match perfectly. 

So one of my next action items is to fix that. 

## Faeries go both ways

Last year's build of the Witch Lights concealed a dirty secret: sprites only had the capacity to move in one direction. 

Basically, a sprite started at `0`, and when in travel mode, it moved by adding 1 to `CurrentPixel`. When `CurrentPixel` was > `NUM_LEDS`, the sprite was marked done. (Or it started at `NUM_LEDS` and traveled towards `0`.)

What I *want*, though, is for faeries to make one long travel move in the "forwards" direction (whichever way that happens to be for this particular sprite), and then make a bunch of small, random moves in either direction, basically flitting about while waiting for people to catch up along the path. 

So that means that all the movement logic needed to be examined and re-written. Which I will not get into here. But basically, it happened. 

[Faeries traveling in both directions][vid3]

[vid3]: https://vimeo.com/271040882

So that's working, too. 

## Reverse the polarity of the neutron flow

The [Witch Lights][wl] have two long-range motion sensors, one at each end. When someone on the "far" end triggers a sensor, we have to spawn a faerie sprite that starts at `NUM_LEDS` (the far end), and travels towards pixel `0`. 

[wl]: https://witchlights.com/

There are lots of details involved in making sprites travel in the "other" direction; for example, when transitioning from travel mode to idle, the idle pixel animation has to run in reverse. And the logic to check whether the sprite is "done" (and can be deleted from memory) has to know the sprite's direction, and so on. 

Or. 

When we run an `Update()` method, when we take what we've animated and send it to the LEDs, what we actually do is call a function called `stripcpy`. That function takes the CRGB struct that we've been manipulating, looks up `currentPixel`, and then uses `memcpy` to write our pixels to the `leds` CRGB struct. 

OK. So. 

What if we told `stripcpy` to count from the *other* direction, and write the CRGB data *backwards*? 

In theory, you'd get a perfect mirror image of the animation you're running. The sprite logic would always run from pixel `0` to `NUM_LEDS`, but it would draw to the pixels as though it was running in the other direction. 

That simplifies the requirement for direction-checking logic all through the various methods of the sprite class, and only requires that sprites be created with a `drawDirection` value, which then gets passed to all `stripcpy` calls. 

That's the theory. I haven't tried it out yet. But I'm eager to try it. If it works, that's a huge simplification, reducing the number of potential logic bugs in Travel mode. 

It would very slightly complicate collision detection, but I'm pretty sure that's easily solved. 

## Up Next

Here's the todo list:

* Add "flit" behavior, so that faeries make long travel moves towards `NUM_LEDS`, then a number of short, random moves in either direction, with short idles, and then move on with another long travel move

* Make `stripcpy` run backwards

* Add jumper-pin modes (set some pins to auto-pullup, and then jumper them to ground to activate various modes for setup and documentation)

* Collision detection

* Adjust `accelerationFactor` and `brakeFactor` randomization, maybe make them values passed to sprites on creation?

* Create subclasses of the current sprite and change the idle animation

* Fix the visual glitch when transitioning from idle to travel modes (FadeColor?)

* Define zones of pixels where faerie sprites should not stop and idle
	* areas where the trail coils around a tree
	* zones at the very beginning and end of pixel strips

* Use collision detection to make faeries more likely to stop and flit around near other faeries

* Double-check memory usage and possible memory leaks caused by my messing with things I do not sufficiently understand

And other things. Secret things. 