# GraphicsTweaks
![GitHub Downloads (specific asset, latest release)](https://img.shields.io/github/downloads/FrozenAlex/GraphicsTweaks/latest/GraphicsTweaks.qmod)
![GitHub Downloads (specific asset, all releases)](https://img.shields.io/github/downloads/FrozenAlex/GraphicsTweaks/GraphicsTweaks.qmod)

Use `qpm-rust s build` to build
Same goes for `qpm-rust s copy` and `qpm-rust s qmod`

## Hot Reloading

To enable hot reloading, you need to uncomment `add_compile_options(-DHotReload)` in `CMakeLists.txt` and build again.

To automatically copy the templates to quest you need to run `npm install` in the project folder and then `npm run watch` to start the watcher, it will automatically copy the bsml files to the quest when they change and the mod will reload them every 0.5s.

## Credits

* [zoller27osu](https://github.com/zoller27osu), [Sc2ad](https://github.com/Sc2ad) and [jakibaki](https://github.com/jakibaki) - [beatsaber-hook](https://github.com/sc2ad/beatsaber-hook)
* [raftario](https://github.com/raftario)
* [Lauriethefish](https://github.com/Lauriethefish), [danrouse](https://github.com/danrouse) and [Bobby Shmurner](https://github.com/BobbyShmurner) for [this template](https://github.com/Lauriethefish/quest-mod-template)
