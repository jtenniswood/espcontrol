import { state } from "../state/app_instance";
import { staticGlobal, type GlobalDescriptors } from "../runtime/globals";
export function installSettingsPhotosSectionModule(): GlobalDescriptors {
    // ── Settings Photo Screensaver Section ─────────────────────────────
    // Shown when the screensaver "Then" action is "Photos". Configures the
    // Home Assistant media-source folder to rotate through, the per-photo
    // duration, and the optional overlay widgets. The connection URL and token
    // live in the Home Assistant connection fields shared with entity
    // suggestions; this card only owns the photo-specific settings.
    function buildPhotoSettingsCard(this: any) {
        var body: any = document.createElement("div");
        body.className = "sp-photos-settings";

        var folderField: any = document.createElement("div");
        folderField.className = "sp-field";
        folderField.appendChild(fieldLabel("Photos Folder", "sp-set-ss-photos-folder"));
        var folderInp: any = document.createElement("input");
        folderInp.type = "text";
        folderInp.className = "sp-input";
        folderInp.id = "sp-set-ss-photos-folder";
        folderInp.placeholder = "media-source://media_source/local/photos";
        folderInp.value = state.photosFolder || "";
        folderField.appendChild(folderInp);
        var folderHelp: any = document.createElement("div");
        folderHelp.className = "sp-help";
        folderHelp.textContent =
            "The Home Assistant media-source folder to show, for example " +
            "media-source://media_source/local/photos for a folder in config/www or media.";
        folderField.appendChild(folderHelp);
        body.appendChild(folderField);
        bindTextPost(folderInp, entityName("screen_saver_photos_folder"), {
            onBlur: function (this: any, value?: any) { state.photosFolder = value; },
            post: function (this: any, value?: any) { postScreensaverPhotosFolder(value); },
        });
        els.setPhotosFolder = folderInp;

        var durationField: any = document.createElement("div");
        durationField.className = "sp-field";
        durationField.appendChild(fieldLabel("Seconds Per Photo", "sp-set-ss-photos-interval"));
        var durationInp: any = document.createElement("input");
        durationInp.type = "number";
        durationInp.min = "5";
        durationInp.max = "3600";
        durationInp.step = "5";
        durationInp.className = "sp-input";
        durationInp.id = "sp-set-ss-photos-interval";
        durationInp.value = String(state.photosInterval || 30);
        durationField.appendChild(durationInp);
        body.appendChild(durationField);
        durationInp.addEventListener("change", function (this: any) {
            var n: any = parseInt(this.value, 10);
            if (!Number.isFinite(n) || n < 5) n = 30;
            if (n > 3600) n = 3600;
            state.photosInterval = n;
            this.value = String(n);
            postScreensaverPhotosInterval(n);
        });
        els.setPhotosInterval = durationInp;

        var shuffleToggle: any = toggleRow("Shuffle", "sp-set-ss-photos-shuffle", state.photosShuffle);
        body.appendChild(shuffleToggle.row);
        shuffleToggle.input.addEventListener("change", function (this: any) {
            state.photosShuffle = this.checked;
            postScreensaverPhotosShuffle(state.photosShuffle);
        });
        els.setPhotosShuffle = shuffleToggle.input;

        var datetimeToggle: any = toggleRow("Show Time", "sp-set-ss-photos-datetime", state.photosShowDatetime);
        body.appendChild(datetimeToggle.row);
        datetimeToggle.input.addEventListener("change", function (this: any) {
            state.photosShowDatetime = this.checked;
            postScreensaverPhotosShowDatetime(state.photosShowDatetime);
        });
        els.setPhotosShowDatetime = datetimeToggle.input;

        var dateToggle: any = toggleRow("Show Date", "sp-set-ss-photos-date", state.photosShowDate);
        body.appendChild(dateToggle.row);
        dateToggle.input.addEventListener("change", function (this: any) {
            state.photosShowDate = this.checked;
            postScreensaverPhotosShowDate(state.photosShowDate);
        });
        els.setPhotosShowDate = dateToggle.input;

        var weatherToggle: any = toggleRow("Show Weather", "sp-set-ss-photos-weather", state.photosShowWeather);
        body.appendChild(weatherToggle.row);
        weatherToggle.input.addEventListener("change", function (this: any) {
            state.photosShowWeather = this.checked;
            syncPhotoWeatherField();
            postScreensaverPhotosShowWeather(state.photosShowWeather);
        });
        els.setPhotosShowWeather = weatherToggle.input;

        var weatherField: any = document.createElement("div");
        weatherField.className = "sp-field";
        weatherField.appendChild(fieldLabel("Weather Entity", "sp-set-ss-photos-weather-entity"));
        var weatherInp: any = entityInput("sp-set-ss-photos-weather-entity", state.photosWeatherEntity, "e.g. weather.home", ["weather"]);
        weatherField.appendChild(weatherInp);
        var weatherGroup: any = document.createElement("div");
        weatherGroup.className = "sp-cond-group";
        weatherGroup.appendChild(weatherField);
        body.appendChild(weatherGroup);
        bindTextPost(weatherInp, entityName("screen_saver_photos_weather_entity"), {
            onBlur: function (this: any, value?: any) { state.photosWeatherEntity = value; },
            post: function (this: any, value?: any) { postScreensaverPhotosWeatherEntity(value); },
        });
        els.setPhotosWeatherEntity = weatherInp;

        function syncPhotoWeatherField(this: any) {
            weatherGroup.classList.toggle("sp-visible", !!state.photosShowWeather);
        }
        syncPhotoWeatherField();
        els.syncPhotoWeatherField = syncPhotoWeatherField;

        // ── Agenda overlay ────────────────────────────────────────────────
        var agendaToggle: any = toggleRow("Show Agenda", "sp-set-ss-photos-agenda", state.photosShowAgenda);
        body.appendChild(agendaToggle.row);
        agendaToggle.input.addEventListener("change", function (this: any) {
            state.photosShowAgenda = this.checked;
            syncPhotoAgendaFields();
            postScreensaverPhotosShowAgenda(state.photosShowAgenda);
        });
        els.setPhotosShowAgenda = agendaToggle.input;

        var agendaGroup: any = document.createElement("div");
        agendaGroup.className = "sp-cond-group";
        body.appendChild(agendaGroup);

        var agendaCalendars: any = calendarListField(
            "sp-set-ss-photos-agenda-entities", state.photosAgendaEntities,
            function (this: any, csv?: any) {
                state.photosAgendaEntities = csv;
                postScreensaverPhotosAgendaEntities(csv);
            });
        var agendaEntitiesField: any = agendaCalendars.field;
        agendaEntitiesField.className = "sp-field";
        var agendaHelp: any = document.createElement("div");
        agendaHelp.className = "sp-help";
        agendaHelp.textContent = "Each calendar's events take its colour on the screensaver.";
        agendaEntitiesField.appendChild(agendaHelp);
        agendaGroup.appendChild(agendaEntitiesField);
        els.setPhotosAgendaEntities = agendaCalendars.firstInput();
        els.setPhotosAgendaCalendars = agendaCalendars;

        var agendaStyleField: any = document.createElement("div");
        agendaStyleField.className = "sp-field";
        agendaStyleField.appendChild(fieldLabel("Agenda Style", "sp-set-ss-photos-agenda-style"));
        var agendaStyleSel: any = document.createElement("select");
        agendaStyleSel.className = "sp-input";
        agendaStyleSel.id = "sp-set-ss-photos-agenda-style";
        ["Next Event", "Today", "Upcoming"].forEach(function (this: any, opt?: any) {
            var o: any = document.createElement("option");
            o.value = opt;
            o.textContent = opt;
            agendaStyleSel.appendChild(o);
        });
        agendaStyleSel.value = state.photosAgendaStyle || "Next Event";
        agendaStyleField.appendChild(agendaStyleSel);
        agendaGroup.appendChild(agendaStyleField);
        agendaStyleSel.addEventListener("change", function (this: any) {
            state.photosAgendaStyle = this.value;
            postScreensaverPhotosAgendaStyle(state.photosAgendaStyle);
        });
        els.setPhotosAgendaStyle = agendaStyleSel;

        var agendaOpacityField: any = document.createElement("div");
        agendaOpacityField.className = "sp-field";
        agendaOpacityField.appendChild(fieldLabel("Agenda Background Opacity (%)", "sp-set-ss-photos-agenda-opacity"));
        var agendaOpacityInp: any = document.createElement("input");
        agendaOpacityInp.type = "number";
        agendaOpacityInp.min = "0";
        agendaOpacityInp.max = "90";
        agendaOpacityInp.step = "5";
        agendaOpacityInp.className = "sp-input";
        agendaOpacityInp.id = "sp-set-ss-photos-agenda-opacity";
        agendaOpacityInp.value = String(state.photosAgendaOpacity != null ? state.photosAgendaOpacity : 45);
        agendaOpacityField.appendChild(agendaOpacityInp);
        agendaGroup.appendChild(agendaOpacityField);
        agendaOpacityInp.addEventListener("change", function (this: any) {
            var n: any = parseInt(this.value, 10);
            if (!Number.isFinite(n) || n < 0) n = 45;
            if (n > 90) n = 90;
            state.photosAgendaOpacity = n;
            this.value = String(n);
            postScreensaverPhotosAgendaOpacity(n);
        });
        els.setPhotosAgendaOpacity = agendaOpacityInp;

        var agendaLimitField: any = document.createElement("div");
        agendaLimitField.className = "sp-field";
        agendaLimitField.appendChild(fieldLabel("Agenda Events (Today/Upcoming)", "sp-set-ss-photos-agenda-limit"));
        var agendaLimitInp: any = document.createElement("input");
        agendaLimitInp.type = "number";
        agendaLimitInp.min = "1";
        agendaLimitInp.max = "10";
        agendaLimitInp.step = "1";
        agendaLimitInp.className = "sp-input";
        agendaLimitInp.id = "sp-set-ss-photos-agenda-limit";
        agendaLimitInp.value = String(state.photosAgendaLimit != null ? state.photosAgendaLimit : 5);
        agendaLimitField.appendChild(agendaLimitInp);
        agendaGroup.appendChild(agendaLimitField);
        agendaLimitInp.addEventListener("change", function (this: any) {
            var n: any = parseInt(this.value, 10);
            if (!Number.isFinite(n) || n < 1) n = 5;
            if (n > 10) n = 10;
            state.photosAgendaLimit = n;
            this.value = String(n);
            postScreensaverPhotosAgendaLimit(n);
        });
        els.setPhotosAgendaLimit = agendaLimitInp;

        var agendaDaysField: any = document.createElement("div");
        agendaDaysField.className = "sp-field";
        agendaDaysField.appendChild(fieldLabel("Agenda Days Ahead", "sp-set-ss-photos-agenda-days"));
        var agendaDaysInp: any = document.createElement("input");
        agendaDaysInp.type = "number";
        agendaDaysInp.min = "1";
        agendaDaysInp.max = "30";
        agendaDaysInp.step = "1";
        agendaDaysInp.className = "sp-input";
        agendaDaysInp.id = "sp-set-ss-photos-agenda-days";
        agendaDaysInp.value = String(state.photosAgendaDays != null ? state.photosAgendaDays : 14);
        agendaDaysField.appendChild(agendaDaysInp);
        var agendaDaysHelp: any = document.createElement("div");
        agendaDaysHelp.className = "sp-help";
        agendaDaysHelp.textContent = "How far ahead to look for events, and how many days the full agenda view shows.";
        agendaDaysField.appendChild(agendaDaysHelp);
        agendaGroup.appendChild(agendaDaysField);
        agendaDaysInp.addEventListener("change", function (this: any) {
            var n: any = parseInt(this.value, 10);
            if (!Number.isFinite(n) || n < 1) n = 14;
            if (n > 30) n = 30;
            state.photosAgendaDays = n;
            this.value = String(n);
            postScreensaverPhotosAgendaDays(n);
        });
        els.setPhotosAgendaDays = agendaDaysInp;

        var agendaHideEmptyRow: any = toggleRow("Hide Empty Days", "sp-set-ss-photos-agenda-hide-empty", state.photosAgendaHideEmpty);
        agendaGroup.appendChild(agendaHideEmptyRow.row);
        agendaHideEmptyRow.input.addEventListener("change", function (this: any) {
            state.photosAgendaHideEmpty = this.checked;
            postScreensaverPhotosAgendaHideEmpty(state.photosAgendaHideEmpty);
        });
        els.setPhotosAgendaHideEmpty = agendaHideEmptyRow.input;

        function syncPhotoAgendaFields(this: any) {
            agendaGroup.classList.toggle("sp-visible", !!state.photosShowAgenda);
        }
        syncPhotoAgendaFields();
        els.syncPhotoAgendaFields = syncPhotoAgendaFields;

        return body;
    }
    return {
        "buildPhotoSettingsCard": staticGlobal(buildPhotoSettingsCard),
    };
}
