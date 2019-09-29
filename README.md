## Maya Motion Path

Our Motion Path tool is the ultimate solution for real-time visualization and editing of your animations in 3D space.

### Features:

* Real-time motion path visualization
* Camera and world space draw modes
* Dockable GUI
* Optional key frames visualization
* Optional “Use Pivots” mode to take modified pivots into account
* Optional key frame tangents visualization
* Optional rotational key frame visualization
* For objects with incoming connections, such as constraints or set driven keys, a baked/non-editable path is shown.
* Lock selection
* Interactive switch for lock selection
* Motion path edit tool
* Motion path draw tool
* With the draw tool on, ctrl+click on a keyframe and drag to activate stroke mode. Closest: move the keys to the closest point on the drawn stroke; Spread: distributes keys uniformly along the drawn point
* Multiple buffer curves
* Buffer Curve to Nurb Curve feature
* Copy 3D key frames positions inside the Maya viewport
* World Paste 3D key frames positions onto other objects
* Offset Paste 3D key frames positions onto other objects
* Customizable settings, such as colors and sizes
* Motion Path settings are saved in maya preferences
* Stroke Mode (Ctrl + click a keyframe and drag with the draw context activated)

If you are planning to use one of our tools in a studio, we would be grateful if you could let us know.

### Known limitations:

* Please use Maya 2017 update 4 or the tool UI will freeze when docked in the Maya UI. Everything works as expected when the UI is not docked, even on earlier Maya updates.
* Frames display does not show correctly On Linux (Viewport 2.0 )
* Weighted paths won’t display aligned-correct curve tangents when drawing using world space mode. Weighted paths won’t displayed tangents in camera space mode.
* Key selection is not integrated fully with Maya undo, it won’t work in case of object deletions and similar actions.
* Marquee selection in the MotionPathEditContext does not work with keys.
* When locking selection, drawing the path for the locked object could be slow depending on the object hierarchy and connections.
* Lock selection mode could be quite slow depending on the hierarchy/network of the locked object.
* Rotational Keys are shown only in conjunction with one or more translation key frames.
* With animation layers a baked/non-editable path will be shown.
* Copy-Paste could not work as expected in some cases: 1) pasting keys on items with a different parent 2) when some world tangent info won’t be available from your source curves 3) when not copying all keys from the original curve
* Copy-Paste only copies translation values, it DOES NOT work with rotations.

## License

This project is licensed under [the LGPL license](http://www.gnu.org/licenses/).

## Contact us

Please feel free to contact us at support@toolchefs.com in case you would like contribute or simply have questions.

### ENJOY!
