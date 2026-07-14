import { liveGlobal, staticGlobal, type GlobalDescriptors } from "../runtime/globals";
export function installAppTestHooks(): GlobalDescriptors {
    function registerEspControlTestHookGroup(this: any, groupName?: any, hooks?: any) {
        if (typeof globalThis === "undefined" || !globalThis.__ESPCONTROL_TEST_HOOKS__)
            return;
        var registry: any = globalThis.__ESPCONTROL_TEST_HOOKS__;
        if (!registry.config)
            registry.config = {};
        if (!registry.groups)
            registry.groups = {};
        if (registry.groups[groupName]) {
            throw new Error("Duplicate ESPControl test hook group: " + groupName);
        }
        registry.groups[groupName] = hooks || {};
        for (var key in registry.groups[groupName]) {
            if (Object.prototype.hasOwnProperty.call(registry.config, key)) {
                throw new Error("Duplicate ESPControl test hook: " + key);
            }
            registry.config[key] = registry.groups[groupName][key];
        }
    }
    return {
        "registerEspControlTestHookGroup": staticGlobal(registerEspControlTestHookGroup),
    };
}
