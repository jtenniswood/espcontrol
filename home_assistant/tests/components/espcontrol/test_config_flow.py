"""Home Assistant-style config-flow tests for the EspControl POC."""

from __future__ import annotations

from ipaddress import IPv4Address

from homeassistant.config_entries import SOURCE_ZEROCONF
from homeassistant.data_entry_flow import FlowResultType
from homeassistant.helpers.service_info.zeroconf import ZeroconfServiceInfo

from custom_components.espcontrol.const import DOMAIN


def _discovery_info(
    device_id: str = "aa:bb:cc:dd:ee:ff", protocol: str = "1"
) -> ZeroconfServiceInfo:
    return ZeroconfServiceInfo(
        ip_address=IPv4Address("192.0.2.10"),
        ip_addresses=[IPv4Address("192.0.2.10")],
        hostname="espcontrol-kitchen.local.",
        name="espcontrol-kitchen._espcontrol._tcp.local.",
        port=6053,
        properties={
            "protocol": protocol,
            "device_id": device_id,
            "mac": device_id,
            "model": "guition-esp32-p4-jc8012p4a1",
            "web_port": "80",
        },
        type="_espcontrol._tcp.local.",
    )


async def test_zeroconf_discovery_creates_entry(hass) -> None:
    """A stable mDNS ID creates one confirmed config entry."""

    result = await hass.config_entries.flow.async_init(
        DOMAIN,
        context={"source": SOURCE_ZEROCONF},
        data=_discovery_info(),
    )
    assert result["type"] is FlowResultType.FORM
    result = await hass.config_entries.flow.async_configure(result["flow_id"], {})
    assert result["type"] is FlowResultType.CREATE_ENTRY
    assert result["data"]["device_id"] == "aa:bb:cc:dd:ee:ff"
    assert result["data"]["web_port"] == 80


async def test_unsupported_protocol_is_ignored(hass) -> None:
    """A future advertisement must not be claimed by an old integration."""

    info = _discovery_info(protocol="99")
    result = await hass.config_entries.flow.async_init(
        DOMAIN,
        context={"source": SOURCE_ZEROCONF},
        data=info,
    )
    assert result["type"] is FlowResultType.ABORT
    assert result["reason"] == "unsupported_protocol"
