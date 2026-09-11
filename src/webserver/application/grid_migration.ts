import { state } from "../state/app_instance";
import { serializeGridOrder } from "../model";
import type { ApplicationLayoutState } from "./application_context";
import type { UiRuntimeState } from "./state";

export interface GridMigrationDependencies {
  readonly renderPreview: () => void;
  readonly renderButtonSettings: () => void;
  readonly postOrder: (value: string) => void;
}

export interface GridMigrationFeature {
  hasConfiguredGrid(): boolean;
  schedule(): void;
}

export function createGridMigrationFeature(
  runtime: UiRuntimeState,
  layout: ApplicationLayoutState,
  dependencies: GridMigrationDependencies,
): GridMigrationFeature {
  function hasConfiguredGrid(): boolean {
    for (let index = 0; index < layout.numSlots; index++) {
      if ((state.grid[index] ?? 0) > 0) return true;
    }
    return false;
  }

  function schedule(): void {
    if (runtime.orderReceived || hasConfiguredGrid()) return;
    clearTimeout(runtime.migrationTimer as any);
    runtime.migrationTimer = setTimeout(() => {
      if (runtime.orderReceived || hasConfiguredGrid()) return;
      let position = 0;
      for (let index = 0; index < layout.numSlots; index++) {
        if (state.buttons[index]?.entity && position < layout.numSlots) {
          state.grid[position] = index + 1;
          position++;
        }
      }
      if (position > 0) {
        dependencies.renderPreview();
        dependencies.renderButtonSettings();
        dependencies.postOrder(serializeGridOrder(state.grid, state.sizes));
      }
    }, 2000);
  }

  return { hasConfiguredGrid, schedule };
}
