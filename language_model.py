from typing import Tuple
from ngram_storage import CStorage


class LanguageModel:
    def __init__(self, storage: CStorage, delta=0.75, eps=1.0):
        self.storage = storage
        self.delta = delta
        self.eps = eps

    def get_word_prob(self, word: str, context: Tuple[str, ...] = ()):
        all_continuations = max(1, self.storage.get_continuations_count(context))
        unique_continuations = self.storage.get_unique_continuations_count(context)

        if unique_continuations == 0:
            if len(context) > 0:
                return self.get_word_prob(word, context[1:])
            else:
                return 1.0

        if all_continuations < 10 and len(context) > 0:
            return self.get_word_prob(word, context[1:])

        prob = max(0.0, self.storage[context + (word,)] - self.delta) / all_continuations
        coef = self.delta * unique_continuations / all_continuations
        if len(context) == 0:
            return prob + coef * self.eps / unique_continuations
        else:
            return prob + coef * self.get_word_prob(word, context[1:])