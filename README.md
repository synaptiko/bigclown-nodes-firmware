# BigClown Firmware customized according to the modules I own

## What do I own, so far:
- one motion detector kit
- one climate monitor kit
- one core module with tags and standard battery module

## Additional customizations:
- all sensors send data more often than in the default firmware (from 15 minutes changed to 1 minute)
- charge level in percents is sent in addition to voltage level
- removed all unnecessary code: button handling, LED blink
- renamed the module name sent as a pairing info

Run `./build.sh` to build all firmware files.
