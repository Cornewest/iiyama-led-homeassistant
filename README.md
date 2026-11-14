# iiyama Android Touchscreen Side-LED Control in Home Assistant

Complete guide to directly control the built-in RGB side-LED bars on **iiyama ProLite Android touchscreens** (e.g., TW1523AS / TW1623AS / TW2223AS series) from **Home Assistant** over ADB — without relying on third-party apps or Tasker.

## Prerequisites: Enable ADB on the Touchscreen
1. Open Android **Settings > About tablet** (or **About device**) on the touchscreen.
2. Tap **Build number** **7 times** until *"You are now a developer!"* appears.
3. Go back to **Settings > System > Developer options**.
4. Enable **USB debugging** and **Network debugging / Wireless ADB** (port `5555`).

## 1. Hardware & Kernel Details
The RGB side-LED strips on these iiyama displays are driven directly by the Rockchip GPIO/PWM kernel driver via `/dev/led_gpio` using `ioctl` calls:

* **Device node:** `/dev/led_gpio`
* **Brightness range:** `0` (off) to `15` (max)
* **Color channels:**
  * `161` = **Red**
  * `162` = **Green**
  * `163` = **Blue**

> **Important:** Writing multiple channels back-to-back without a short delay (`50ms`) causes the kernel driver to drop the first commands. Always include `sleep 0.05` between channel calls.

---

## 2. Install the `ledctl` Binary on the Display

Because standard Android shell tools (`echo` / `toybox`) cannot send raw `ioctl` requests to `/dev/led_gpio`, a tiny native ARM64 binary (`ledctl`) is included in this repository (`ledctl.c` source code is also provided for transparency).

1. Download **`ledctl`** from this repository.
2. Push it to `/data/local/tmp/ledctl` on the touchscreen and make it executable via ADB (replace `<DISPLAY_IP>` with your touchscreen's IP address):

```bash
adb connect <DISPLAY_IP>:5555
adb push ledctl /data/local/tmp/ledctl
adb shell "su 0 chmod 755 /data/local/tmp/ledctl"
```

### Quick CLI Test
```bash
# Red on, Green & Blue off
adb shell "su 0 sh -c '/data/local/tmp/ledctl 162 0; sleep 0.05; /data/local/tmp/ledctl 163 0; sleep 0.05; /data/local/tmp/ledctl 161 15'"
```

*(Disconnect your manual ADB session afterwards with `adb disconnect` so Home Assistant can connect cleanly).*

---

## 3. Keep LEDs Powered While Screen Is Dimmed / Screensaver Active
When Android turns the display completely off (`screen off`), the kernel suspends the GPIO/PWM driver and turns off the side-LED strips.

To keep the LEDs lit 24/7 while dimming the screen:
1. Disable Android system sleep via ADB:
   ```bash
   adb shell "su 0 settings put global stay_on_while_plugged_in 7 && su 0 settings put system screen_off_timeout 2147483647"
   ```
2. In **Fully Kiosk Browser** (or your kiosk app):
   * Enable **Keep Screen On**.
   * Use the **Screensaver** (black screen or moving clock URL) with **Turn Screen Off in Screensaver** disabled.

---

## 4. Home Assistant Configuration

### Step 1: Create an Input Select Helper
Create a **Dropdown (`input_select`)** helper in Home Assistant (**Settings > Devices & Services > Helpers**):
* **Name:** `iiyama LED Kleur` (`input_select.iiyama_led_kleur`)
* **Options:**
  * `Uit`
  * `Rood`
  * `Groen`
  * `Blauw`
  * `Geel`
  * `Paars`
  * `Cyaan`
  * `Wit`

### Step 2: Automation (`androidtv.adb_command`)
Ensure the **Android Debug Bridge (`androidtv`)** integration is connected to your touchscreen (`media_player.<your_display>`), then create a new automation and paste this YAML (replace `media_player.<your_display>` with your actual ADB media player entity):

```yaml
alias: iiyama LED Strip Control
description: Controls the built-in side-LED strips on the iiyama Android touchscreen
mode: restart
triggers:
  - trigger: state
    entity_id: input_select.iiyama_led_kleur
actions:
  - action: androidtv.adb_command
    target:
      entity_id: media_player.<your_display>
    data:
      command: >
        {% set k = states('input_select.iiyama_led_kleur') | lower %}
        {% if k in ['rood', 'red'] %}
          su 0 sh -c '/data/local/tmp/ledctl 162 0; sleep 0.05; /data/local/tmp/ledctl 163 0; sleep 0.05; /data/local/tmp/ledctl 161 15'
        {% elif k in ['groen', 'green'] %}
          su 0 sh -c '/data/local/tmp/ledctl 161 0; sleep 0.05; /data/local/tmp/ledctl 163 0; sleep 0.05; /data/local/tmp/ledctl 162 15'
        {% elif k in ['blauw', 'blue'] %}
          su 0 sh -c '/data/local/tmp/ledctl 161 0; sleep 0.05; /data/local/tmp/ledctl 162 0; sleep 0.05; /data/local/tmp/ledctl 163 15'
        {% elif k in ['geel', 'yellow'] %}
          su 0 sh -c '/data/local/tmp/ledctl 163 0; sleep 0.05; /data/local/tmp/ledctl 161 15; sleep 0.05; /data/local/tmp/ledctl 162 15'
        {% elif k in ['paars', 'purple', 'magenta'] %}
          su 0 sh -c '/data/local/tmp/ledctl 162 0; sleep 0.05; /data/local/tmp/ledctl 161 15; sleep 0.05; /data/local/tmp/ledctl 163 15'
        {% elif k in ['cyaan', 'cyan'] %}
          su 0 sh -c '/data/local/tmp/ledctl 161 0; sleep 0.05; /data/local/tmp/ledctl 162 15; sleep 0.05; /data/local/tmp/ledctl 163 15'
        {% elif k in ['wit', 'white'] %}
          su 0 sh -c '/data/local/tmp/ledctl 161 15; sleep 0.05; /data/local/tmp/ledctl 162 15; sleep 0.05; /data/local/tmp/ledctl 163 15'
        {% else %}
          su 0 sh -c '/data/local/tmp/ledctl 161 0; sleep 0.05; /data/local/tmp/ledctl 162 0; sleep 0.05; /data/local/tmp/ledctl 163 0'
        {% endif %}
```

---

## 5. Example Use Cases
Once exposed as `input_select.iiyama_led_kleur`, you can trigger visual notifications from any Home Assistant automation:
* **Trash collection reminder:** Turn LEDs **Red** the evening before pickup until confirmed on the dashboard, then revert to **White**.
* **Screen cleaning mode:** Turn LEDs **Cyan** during a 30-second screen lock countdown, then restore the previous color.
* **Doorbell / Alarm / Appliance alerts:** Switch colors based on sensor states even while a black or clock screensaver is active.
