
# How to add this plugin to an Unreal Engine project

## 1. Create the `Plugins` folder

In the root folder of your Unreal Engine project, there should be a `Plugins` folder.

Example:

```text
YourProject/
├── YourProject.uproject
├── Content/
├── Source/
└── Plugins/
```

If the `Plugins` folder does not exist, create it manually.


## 2. Clone the plugin into the `Plugins` folder

Open a terminal inside the `Plugins` folder of your Unreal Engine project:

```bash
cd path/to/YourProject/Plugins
```

Clone the plugin repository:

```bash
git clone https://github.com/FeadenGlow/SimWeapons.git SimWeapons
```

After that, the structure should look like this:

```text
YourProject/
└── Plugins/
    └── SimWeapons/
        ├── SimWeapons.uplugin
        ├── Source/
        ├── Resources/
        └── README.md
```


## 3. Open the Unreal Engine project

Open your project file:

```text
YourProject.uproject
```

If Unreal Engine asks to rebuild modules, click:

```text
Yes
```

Unreal Engine will try to build the C++ module of the plugin.


## 4. Enable the plugin

In Unreal Editor, open:

```text
Edit → Plugins
```

Search for:

```text
SimWeapons
```

Make sure the plugin is enabled.

If Unreal Engine asks to restart the editor, restart it.


## How to check that the plugin works

The plugin is connected correctly if:

1. The Unreal Engine project opens without errors.
2. `SimWeapons` is visible in `Edit → Plugins`.
3. The plugin is enabled.
4. The project compiles without errors.
