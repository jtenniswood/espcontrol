import { liveGlobal, type GlobalDescriptors } from "../runtime/globals";
export function registerAgendaCardTypes(): GlobalDescriptors {
    var AGENDA_DAYS_OPTION: any = "agenda_days";
    var AGENDA_DAYS_DEFAULT: any = "14";
    var AGENDA_HIDE_EMPTY_OPTION: any = "agenda_hide_empty";
    // Read-only agenda card: shows the next few upcoming events from one or
    // more Home Assistant calendars, grouped by day.
    var AGENDA_CARD_METADATA: any = {
        entity: {
            label: "Calendar Entities",
            idSuffix: "entity",
            placeholder: "e.g. calendar.family, calendar.work",
            domains: function (this: any) { return cardContractDomains("agenda"); },
            multi: true,
            bindName: "entity",
            rerender: true,
            requiredMessage: "Add at least one calendar entity before saving.",
        },
        labelField: {
            label: "Label",
            idSuffix: "label",
            placeholder: "e.g. Agenda",
        },
        preview: {
            badge: "calendar-clock",
        },
    };
    registerButtonType("agenda", {
        label: function (this: any) { return cardContractCardLabel("agenda"); },
        allowInSubpage: function (this: any) { return cardContractAllowInSubpage("agenda"); },
        pickerKey: function (this: any) { return cardContractPickerKey("agenda"); },
        hidden: function (this: any) { return cardContractHidden("agenda"); },
        hideLabel: true,
        defaultConfig: function (this: any) { return cardContractDefaultConfig("agenda"); },
        cardMetadata: AGENDA_CARD_METADATA,
        onSelect: function (this: any, b?: any) {
            b.icon = "Auto";
            b.icon_on = "Auto";
            b.sensor = "";
            b.unit = "";
            b.precision = "";
            b.options = "";
        },
        renderSettings: function (this: any, panel?: any, b?: any, slot?: any, helpers?: any) {
            var calendars: any = calendarListField(
                helpers.idPrefix + "entity", b.entity,
                function (this: any, csv?: any) {
                    b.entity = csv;
                    helpers.saveField("entity", csv);
                });
            var calendarHelp: any = document.createElement("div");
            calendarHelp.className = "sp-help";
            calendarHelp.textContent =
                "Pick a calendar per row; its events take that row's colour on the panel.";
            calendars.field.appendChild(calendarHelp);
            panel.appendChild(calendars.field);
            helpers.requireField(calendars.firstInput(),
                                 AGENDA_CARD_METADATA.entity.requiredMessage);
            helpers.renderCardTextField(panel, b, helpers, AGENDA_CARD_METADATA.labelField);
            // Blank means the contract default, so an untouched card stores no
            // option at all.
            var daysField: any = helpers.renderCardNumberField(panel, b, helpers, {
                label: "Days Ahead",
                idSuffix: "agenda-days",
                min: 1,
                max: 30,
                step: 1,
                placeholder: AGENDA_DAYS_DEFAULT,
                value: function (this: any) {
                    var days: any = configOptionValue(b && b.options, AGENDA_DAYS_OPTION);
                    return days === AGENDA_DAYS_DEFAULT ? "" : days;
                },
            });
            daysField.input.addEventListener("change", function (this: any) {
                var raw: any = String(daysField.input.value).trim();
                var days: any = parseInt(raw, 10);
                var stored: any = "";
                if (raw !== "" && Number.isFinite(days)) {
                    if (days < 1) days = 1;
                    if (days > 30) days = 30;
                    stored = String(days) === AGENDA_DAYS_DEFAULT ? "" : String(days);
                }
                daysField.input.value = stored;
                b.options = setConfigOptionValue(b.options, AGENDA_DAYS_OPTION, stored);
                helpers.saveField("options", b.options);
            });
            var hideEmpty: any = helpers.toggleRow(
                "Hide Empty Days", helpers.idPrefix + "agenda-hide-empty",
                configOptionEnabled(b && b.options, AGENDA_HIDE_EMPTY_OPTION));
            panel.appendChild(hideEmpty.row);
            hideEmpty.input.addEventListener("change", function (this: any) {
                b.options = setConfigOption(b.options, AGENDA_HIDE_EMPTY_OPTION, this.checked);
                helpers.saveField("options", b.options);
            });
        },
        renderPreview: function (this: any, b?: any, helpers?: any) {
            var label: any = b.label || "Agenda";
            return {
                iconHtml: '<span class="sp-btn-icon mdi mdi-calendar-clock"></span>',
                labelHtml: cardBadgeLabelHtml(helpers, label, AGENDA_CARD_METADATA.preview.badge),
            };
        },
    });
    return {
        "AGENDA_CARD_METADATA": liveGlobal(() => AGENDA_CARD_METADATA, (value?: any) => { AGENDA_CARD_METADATA = value; }),
    };
}
