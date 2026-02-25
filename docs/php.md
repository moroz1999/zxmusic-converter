# PHP

## Core Principles
- Stick to Domain Driven Design and Onion Architecture for all NEWLY written code.
- Use rules of SOLID principles.
- Use strict mode.
- ALWAYS use full variable names.
- ALWAYS use imports (`use`) for classes and namespaces. Fully qualified names in the code are prohibited.
- NEVER add unnecessary duplicate type casting.
- Use typed constants (e.g., `public const int MY_CONSTANT = 1;`).
- Place constants and variables at the beginning of the class, before any methods.

## Immutability and Type Safety
- DTOs must be 100% immutable. Use `readonly class` and constructor property promotion.
- Services and other stateless classes should be marked as `readonly class` if all their properties are immutable (e.g., dependencies injected via constructor). When a class is `readonly`, individual `readonly` modifiers on properties are redundant and should be omitted.
- Do NOT write PHPDoc `/** @var ... */` for `getService` calls if the class name is explicitly provided as the first argument (e.g. `getService(MyService::class)`). Modern IDEs and Psalm can infer the type from the class string.

## Coding Style
- Avoid "comment ladders" (multiple sequential comments describing every line of code).
- Avoid inline method calls in conditions if they represent a state. Assign the result to a descriptive variable instead:
  ```php
  $isEditable = $element->isEditable();
  if ($isEditable) { ... }
  ```
  instead of `if ($element->isEditable()) { ... }`.
- Do NOT add a BOM header to any file.
- ALWAYS use strict comparisons (`===`, `!==`). Avoid "falsy" and "truthy" checks (e.g., use `if ($var === true)` instead of `if ($var)`).
- Do NOT use magic numbers. Use class constants for single values or Enums for sets of related values.

## Psalm
- NEVER use @psalm-suppress. Instead, add clear and minimal type annotations.
- Annotate magic variables and methods in original legacy classes.
