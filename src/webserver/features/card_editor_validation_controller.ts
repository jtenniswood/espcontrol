export interface RequiredCardField {
  readonly value: unknown;
  readonly active?: boolean;
  readonly present?: boolean;
}

export interface RequiredCardFieldValidation {
  readonly valid: boolean;
  readonly firstInvalidIndex: number;
}

export interface CardEditorValidationResult {
  readonly valid: boolean;
  readonly reason: "required-field" | "config-size" | "image-limit" | null;
  readonly firstInvalidIndex: number;
}

/** Keeps save decisions independent of the DOM that displays their errors. */
export class CardEditorValidationController {
  validateRequiredFields(fields: readonly RequiredCardField[]): RequiredCardFieldValidation {
    for (let index = 0; index < fields.length; index += 1) {
      const field = fields[index]!;
      if (field.active === false) continue;
      if (field.present !== undefined) {
        if (field.present) continue;
        return { valid: false, firstInvalidIndex: index };
      }
      if (String(field.value || "").trim()) continue;
      return { valid: false, firstInvalidIndex: index };
    }
    return { valid: true, firstInvalidIndex: -1 };
  }

  validateSave(options: {
    readonly fields: readonly RequiredCardField[];
    readonly isSubpage: boolean;
    readonly serializedConfigLength: number;
    readonly imageCardCount: number;
    readonly imageCardCapacity: number;
  }): CardEditorValidationResult {
    const required = this.validateRequiredFields(options.fields);
    if (!required.valid) return { valid: false, reason: "required-field", firstInvalidIndex: required.firstInvalidIndex };
    if (!options.isSubpage && options.serializedConfigLength > 255) {
      return { valid: false, reason: "config-size", firstInvalidIndex: -1 };
    }
    if (options.imageCardCount > options.imageCardCapacity) {
      return { valid: false, reason: "image-limit", firstInvalidIndex: -1 };
    }
    return { valid: true, reason: null, firstInvalidIndex: -1 };
  }
}

export function createCardEditorValidationController(): CardEditorValidationController {
  return new CardEditorValidationController();
}
