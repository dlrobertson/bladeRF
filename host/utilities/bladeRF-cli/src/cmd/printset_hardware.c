/*
 * This file is part of the bladeRF project
 *
 * Copyright (C) 2013-2018 Nuand LLC
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */
#include "cmd.h"
#include "printset.h"
#include <inttypes.h>
#include <time.h>

/* hardware */
static void _print_rfic(struct cli_state *state)
{
    int status;

    float temperature;
    uint8_t ctrlout, reg35, reg36;

    /* Temperature */
    status = bladerf_get_rfic_temperature(state->dev, &temperature);
    if (status < 0) {
        return;
    }

    /* CTRL_OUT pins */
    status = bladerf_get_rfic_ctrl_out(state->dev, &ctrlout);
    if (status < 0) {
        return;
    }

    /* Control Output Pointer */
    status = bladerf_get_rfic_register(state->dev, 0x35, &reg35);
    if (status < 0) {
        return;
    }

    /* Control Output Enable */
    status = bladerf_get_rfic_register(state->dev, 0x36, &reg36);
    if (status < 0) {
        return;
    }

    printf("    RFIC status:\n");
    printf("      Temperature:  %4.1f °C\n", temperature);
    printf("      CTRL_OUT:     0x%02x (0x035=0x%02x, 0x036=0x%02x)\n", ctrlout,
           reg35, reg36);
}

static void _print_power_source_bladerf2(struct cli_state *state)
{
    int status;

    bladerf_power_sources src;

    status = bladerf_get_power_source(state->dev, &src);
    if (status < 0) {
        return;
    }

    printf("    Power source:   ");
    switch (src) {
        case BLADERF_PS_DC:
            printf("DC Barrel\n");
            break;
        case BLADERF_PS_USB_VBUS:
            printf("USB Bus\n");
            break;
        default:
            printf("Unknown. May require manual observation.\n");
            break;
    }
}

static void _print_power_monitor_bladerf2(struct cli_state *state)
{
    int status;

    float vbus, iload, pload;

    status =
        bladerf_get_pmic_register(state->dev, BLADERF_PMIC_VOLTAGE_BUS, &vbus);
    if (status < 0) {
        return;
    }

    status =
        bladerf_get_pmic_register(state->dev, BLADERF_PMIC_CURRENT, &iload);
    if (status < 0) {
        return;
    }

    status = bladerf_get_pmic_register(state->dev, BLADERF_PMIC_POWER, &pload);
    if (status < 0) {
        return;
    }

    printf("    Power monitor:  %g V, %g A, %g W\n", vbus, iload, pload);
}

static char const *_rfic_rx_portstr(uint32_t port)
{
    const char *pstrs[] = { "A_BAL",  "B_BAL",   "C_BAL", "A_N", "A_P",
                            "B_N",    "B_P",     "C_N",   "C_P", "TXMON1",
                            "TXMON2", "TXMON12", NULL };

    if (port > ARRAY_SIZE(pstrs)) {
        return NULL;
    }

    return pstrs[port];
}

static char const *_rfic_tx_portstr(uint32_t port)
{
    const char *pstrs[] = { "TXA", "TXB", NULL };

    if (port > ARRAY_SIZE(pstrs)) {
        return NULL;
    }

    return pstrs[port];
}

static char const *_rfswitch_portstr(uint32_t port)
{
    const char *pstrs[] = { "OPEN", "RF2(A)", "RF3(B)", NULL };

    if (port > ARRAY_SIZE(pstrs)) {
        return NULL;
    }

    return pstrs[port];
}

static void _print_rfconfig(struct cli_state *state)
{
    int status;

    bladerf_rf_switch_config config;

    status = bladerf_get_rf_switch_config(state->dev, &config);
    if (status < 0) {
        return;
    }

    printf("    RF switch config:\n");
    printf("      TX1: RFIC 0x%x (%-7s) => SW 0x%x (%-7s)\n",
           config.tx1_rfic_port, _rfic_tx_portstr(config.tx1_rfic_port),
           config.tx1_spdt_port, _rfswitch_portstr(config.tx1_spdt_port));
    printf("      TX2: RFIC 0x%x (%-7s) => SW 0x%x (%-7s)\n",
           config.tx2_rfic_port, _rfic_tx_portstr(config.tx2_rfic_port),
           config.tx2_spdt_port, _rfswitch_portstr(config.tx2_spdt_port));
    printf("      RX1: RFIC 0x%x (%-7s) <= SW 0x%x (%-7s)\n",
           config.rx1_rfic_port, _rfic_rx_portstr(config.rx1_rfic_port),
           config.rx1_spdt_port, _rfswitch_portstr(config.rx1_spdt_port));
    printf("      RX2: RFIC 0x%x (%-7s) <= SW 0x%x (%-7s)\n",
           config.rx2_rfic_port, _rfic_rx_portstr(config.rx2_rfic_port),
           config.rx2_spdt_port, _rfswitch_portstr(config.rx2_spdt_port));
}

int print_hardware(struct cli_state *state, int argc, char **argv)
{
    if (ps_is_board(state->dev, BOARD_BLADERF2)) {
        printf("  Hardware status:\n");
        _print_rfic(state);
        _print_power_source_bladerf2(state);
        _print_power_monitor_bladerf2(state);
        _print_rfconfig(state);
    }

    return CLI_RET_OK;
}

/* clock_ref */
int print_clock_ref(struct cli_state *state, int argc, char **argv)
{
    int rv   = CLI_RET_OK;
    int *err = &state->last_lib_error;
    int status;

    bool enabled = false;
    bool locked  = false;

    status = bladerf_get_pll_enable(state->dev, &enabled);
    if (status < 0) {
        *err = status;
        rv   = CLI_RET_LIBBLADERF;
        goto out;
    }

    nanosleep( &(const struct timespec){0, 200000000L}, NULL );

    status = bladerf_get_pll_lock_state(state->dev, &locked);
    if (status < 0) {
        *err = status;
        rv   = CLI_RET_LIBBLADERF;
        goto out;
    }

    printf("  Clock reference: %s%s\n", enabled ? "REFIN to ADF4002" : "none",
           enabled ? (locked ? " (locked)" : " (unlocked!!)") : "");

out:
    return rv;
}

int set_clock_ref(struct cli_state *state, int argc, char **argv)
{
    int rv   = CLI_RET_OK;
    int *err = &state->last_lib_error;
    int status;
    bool val;

    if (argc != 3) {
        printf("Usage: set clock_ref <on|off>\n");
        rv = CLI_RET_NARGS;
        goto out;
    }

    status = str2bool(argv[2], &val);
    if (status < 0) {
        cli_err_nnl(state, argv[0], "Invalid clock_ref value (%s)\n", argv[2]);
        rv = CLI_RET_INVPARAM;
        goto out;
    }

    status = bladerf_set_pll_enable(state->dev, val);
    if (status < 0) {
        *err = status;
        rv   = CLI_RET_LIBBLADERF;
        goto out;
    }

    rv = print_clock_ref(state, 2, NULL);

out:
    return rv;
}

/* refin_freq */
int print_refin_freq(struct cli_state *state, int argc, char **argv)
{
    int rv   = CLI_RET_OK;
    int *err = &state->last_lib_error;
    int status;

    bool enabled;
    uint64_t refclk;

    status = bladerf_get_pll_enable(state->dev, &enabled);
    if (status < 0) {
        *err = status;
        rv   = CLI_RET_LIBBLADERF;
        goto out;
    }

    if (!enabled) {
        goto out;
    }

    status = bladerf_get_pll_refclk(state->dev, &refclk);
    if (status < 0) {
        *err = status;
        rv   = CLI_RET_LIBBLADERF;
        goto out;
    }

    printf("  REFIN frequency: %" PRIu64 " Hz\n", refclk);

out:
    return rv;
}

int set_refin_freq(struct cli_state *state, int argc, char **argv)
{
    int rv   = CLI_RET_OK;
    int *err = &state->last_lib_error;
    int status;

    const struct bladerf_range *range = NULL;
    uint64_t freq;
    bool ok;

    if (argc != 3) {
        printf("Usage: set refin_freq <frequency>\n");
        rv = CLI_RET_NARGS;
        goto out;
    }

    /* Get valid frequency range */
    status = bladerf_get_pll_refclk_range(state->dev, &range);
    if (status < 0) {
        *err = status;
        rv   = CLI_RET_LIBBLADERF;
        goto out;
    }

    /* Parse frequency */
    freq = str2uint64_suffix(argv[2], range->min, range->max, freq_suffixes,
                             NUM_FREQ_SUFFIXES, &ok);
    if (!ok) {
        cli_err_nnl(state, argv[0], "Invalid refin_freq value (%s)\n", argv[2]);
        rv = CLI_RET_INVPARAM;
        goto out;
    }

    status = bladerf_set_pll_refclk(state->dev, freq);
    if (status < 0) {
        *err = status;
        rv   = CLI_RET_LIBBLADERF;
        goto out;
    }

    rv = print_refin_freq(state, argc, argv);

out:
    return rv;
}

/* clock_out */
int print_clock_out(struct cli_state *state, int argc, char **argv)
{
    int rv   = CLI_RET_OK;
    int *err = &state->last_lib_error;
    int status;

    bool enabled;

    status = bladerf_get_clock_output(state->dev, &enabled);
    if (status < 0) {
        *err = status;
        rv   = CLI_RET_LIBBLADERF;
        goto out;
    }

    printf("  Clock output:    %s\n",
           (enabled ? "Enabled via CLKOUT" : "Disabled"));

out:
    return rv;
}

int set_clock_out(struct cli_state *state, int argc, char **argv)
{
    int rv   = CLI_RET_OK;
    int *err = &state->last_lib_error;
    int status;

    bool val;

    if (argc != 3) {
        printf("usage: set clock_out <bool>\n");
        rv = CLI_RET_NARGS;
        goto out;
    }

    status = str2bool(argv[argc - 1], &val);
    if (status < 0) {
        cli_err_nnl(state, argv[0], "Invalid clock_out value (%s)\n",
                    argv[argc - 1]);
        rv = CLI_RET_INVPARAM;
        goto out;
    }

    status = bladerf_set_clock_output(state->dev, val);
    if (status < 0) {
        *err = status;
        rv   = CLI_RET_LIBBLADERF;
        goto out;
    }

    rv = print_clock_out(state, 0, NULL);

out:
    return rv;
}

/* clock_sel */
int print_clock_sel(struct cli_state *state, int argc, char **argv)
{
    int rv   = CLI_RET_OK;
    int *err = &state->last_lib_error;
    int status;

    const char *clock_str;
    bladerf_clock_select clock_sel;

    status = bladerf_get_clock_select(state->dev, &clock_sel);
    if (status < 0) {
        *err = status;
        rv   = CLI_RET_LIBBLADERF;
        goto out;
    }

    switch (clock_sel) {
        case CLOCK_SELECT_ONBOARD:
            clock_str = "Onboard VCTCXO";
            break;
        case CLOCK_SELECT_EXTERNAL:
            clock_str = "External via CLKIN";
            break;
        default:
            clock_str = "Unknown";
            break;
    }

    printf("  Clock input:     %s\n", clock_str);

out:
    return rv;
}

int set_clock_sel(struct cli_state *state, int argc, char **argv)
{
    int rv   = CLI_RET_OK;
    int *err = &state->last_lib_error;
    int status;

    bladerf_clock_select clock_sel;

    if (argc != 3) {
        rv = CLI_RET_NARGS;
        goto out;
    }

    if (!strcasecmp(argv[2], "ONBOARD")) {
        clock_sel = CLOCK_SELECT_ONBOARD;
    } else if (!strcasecmp(argv[2], "EXTERNAL")) {
        clock_sel = CLOCK_SELECT_EXTERNAL;
    } else {
        cli_err_nnl(state, argv[0], "Invalid clock source selection (%s)\n",
                    argv[2]);
        rv = CLI_RET_INVPARAM;
        goto out;
    }

    status = bladerf_set_clock_select(state->dev, clock_sel);
    if (status < 0) {
        *err = status;
        rv   = CLI_RET_LIBBLADERF;
        goto out;
    }

    rv = print_clock_sel(state, 2, argv);

out:
    return rv;
}

/* biastee */
static int _do_print_biastee(struct cli_state *state,
                             bladerf_channel ch,
                             void *arg)
{
    int rv   = CLI_RET_OK;
    int *err = &state->last_lib_error;
    int status;

    bool enable;

    status = bladerf_get_bias_tee(state->dev, ch, &enable);
    if (status < 0) {
        *err = status;
        rv   = CLI_RET_LIBBLADERF;
        goto out;
    }

    printf("  Bias Tee (%s): %s\n", channel2str(ch), enable ? "on" : "off");

out:
    return rv;
}

int print_biastee(struct cli_state *state, int argc, char **argv)
{
    int rv = CLI_RET_OK;

    bladerf_channel ch = BLADERF_CHANNEL_INVALID;

    if (argc < 2 || argc > 3) {
        printf("Usage: print biastee [<channel>]\n");
        rv = CLI_RET_NARGS;
        goto out;
    } else if (3 == argc) {
        ch = str2channel(argv[2]);
        if (BLADERF_CHANNEL_INVALID == ch) {
            cli_err_nnl(state, argv[0], "Invalid channel (%s)\n", argv[2]);
            rv = CLI_RET_INVPARAM;
            goto out;
        }
    }

    rv = ps_foreach_chan(state, true, true, ch, _do_print_biastee, NULL);

out:
    return rv;
}

static int _do_set_biastee(struct cli_state *state,
                           bladerf_channel ch,
                           void *arg)
{
    int rv   = CLI_RET_OK;
    int *err = &state->last_lib_error;
    int status;

    bool *barg = arg;

    status = bladerf_set_bias_tee(state->dev, ch, *barg);
    if (status < 0) {
        *err = status;
        rv   = CLI_RET_LIBBLADERF;
        goto out;
    }

    rv = _do_print_biastee(state, ch, NULL);

out:
    return rv;
}

int set_biastee(struct cli_state *state, int argc, char **argv)
{
    /* Usage: set biastee [<rx|tx>] <0|1> */
    int rv = CLI_RET_OK;
    int status;

    bladerf_channel ch = BLADERF_CHANNEL_INVALID;
    bool val;

    if (argc < 3 || argc > 4) {
        printf("Usage: set biastee [channel] <bool>\n");
        rv = CLI_RET_NARGS;
        goto out;
    } else if (4 == argc) {
        ch = str2channel(argv[2]);
        if (BLADERF_CHANNEL_INVALID == ch) {
            cli_err_nnl(state, argv[0], "Invalid channel (%s)\n", argv[2]);
            rv = CLI_RET_INVPARAM;
            goto out;
        }
    }

    status = str2bool(argv[argc - 1], &val);
    if (status < 0) {
        cli_err_nnl(state, argv[0], "Invalid biastee value (%s)\n",
                    argv[argc - 1]);
        rv = CLI_RET_INVPARAM;
        goto out;
    }

    rv = ps_foreach_chan(state, true, true, ch, _do_set_biastee, &val);

out:
    return rv;
}
