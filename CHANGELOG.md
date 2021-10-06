# HetrickCV Changelog

## 2.0.0 Dev
- Add Binary Gate module.
- Add Binary Noise module.
- Add Gingerbread Chaos module.
- Add Mid/Side module.
- Add XY<->Polar module.
- Add Gamma DSP library requirement. This is an excellent DSP library by Lance Putnam, a former colleague from grad school. We use this library as the foundation for Unfiltered Audio's DSP.
- Resaved all panels with Effra typeface instead of Gibson (very similar, but o and p rendering is fixed on export). Redid all panels in Affinity Designer instead of Adobe Illustrator.
- Fixed missing knob parameter names.

## 1.1.0
- I suppose that I forgot to update this CHANGELOG for 1.0.0. Whoops!
- Added Min-Max module.
- Waveshaper is now polyphonic (thanks, nickfeisst!)
- Random Gates now output 10V instead of 5V (thanks, giogramegna!)

## 0.5.3
- BREAKING CHANGE: Removed Boolean Logic (2-input). The 3-input version will behave identically now if nothing is connected to the third input.
- Added Comparator module.
- Added Delta module.
- Added internal hysteresis to Boolean Logic. This will reduce gate jitter when using non-gate inputs.
- Added this changelog.

## 0.5.2 (November 28, 2017)
- Added Blank Panel. Right-click to select between five designs.
- Expanded Gate Combiner to have eight inputs. Massive code cleanup.
- Added input normalization to Gate Junction.

## 0.5.1 (November 21, 2017)
- Fixes for Gate Junction initialization.

## 0.5.0 (November 18, 2017)
- Initial release.