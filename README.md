# QTmusicplayer

Qt/QML music player with a project-local SQLite music library. The database and
all media files are stored inside the project so the folder can be moved to
another computer without changing drive letters or absolute paths.

## Portable Library Layout

- Database: `AppleMusic/data/music_library.db`
- Media files: `AppleMusic/media/`
- Default cover: `AppleMusic/default_cover.svg`

The `songs` table stores only relative paths such as:

- `media/我是如此相信.mp3`
- `media/我是如此相信.png`
- `media/我是如此相信.lrc`
- `media/我是如此相信.mp4`

At runtime, `MusicManager` resolves those paths from the current project/app
folder with Qt path APIs, then converts local files with `QUrl::fromLocalFile`.
This keeps covers, audio, lyrics, and MV files working after copying the project
to a different disk or a folder with Chinese characters/spaces.

## Move To Another Computer

1. Install Git LFS on the target computer.
2. Clone the repository:

   ```powershell
   git clone https://github.com/JingYuXu-experts/QTmusicplayer.git
   cd QTmusicplayer
   git lfs pull
   ```

3. Open `AppleMusic/CMakeLists.txt` in Qt Creator.
4. Configure with Qt 6.4 or newer and build the app.
5. Run the app. Do not edit paths in the database; keep `AppleMusic/data` and
   `AppleMusic/media` together.

## Adding Music

Place new files in `AppleMusic/media/`. Use matching base names when possible:

- `Song Name.mp3`
- `Song Name.png`
- `Song Name.lrc` or `Song Name.txt`
- `Song Name.mp4`

When the app starts, it creates the database if missing, repairs missing media
paths when files are moved inside `media/`, and adds new `.mp3` files that are
not yet in the database.

## Git Notes

Large media files are tracked with Git LFS through `.gitattributes`:

- `*.mp3`
- `*.mp4`
- `*.png`
- `*.rar`

Build output and Qt Creator user settings are ignored through `.gitignore`, so a
new computer should rebuild locally instead of reusing another machine's build
directory.
